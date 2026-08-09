// ─────────────────────────────────────────────────────────────────────────────
// Tech Aim — deterministic target emulator (Modbus TCP).
//
// WHY THIS EXISTS
// FALSE-SHOT-001 is fixed in code but its FAULT CONDITION has never been
// exercised: the RC2d physical run returned a clean hardware counter of zero,
// so the baseline-adoption safety path never ran. The fault is intermittent,
// so we cannot wait for it to reappear on a range day.
//
// This serves the REAL register map over Modbus TCP so the REAL application
// connects to it and runs its production acquisition path unchanged - no
// duplicated algorithm, no injected fake, no production code modified. What
// the application does against this emulator is what it does against a target.
//
// REGISTER MAP - read from the application, not invented
//   4096  (2 regs)  hardware/microphone check. Non-zero => "connected".
//                   isHardwareConnected() returns false when both read 0.
//   8192  (2 regs)  [1] = shot counter. THE value acquisition watches.
//   8193  (write)   counter reset. Scenario B ACKNOWLEDGES but ignores it.
//   8196  (write)   motor on          } paper feed, written by MotorThread
//   8197  (write)   motor duration    }
//   8198  (r/w)     motor trigger / status
//   16376 + 8*i (2 regs)  shot i coordinates: [1] = x, [0] = y
//
// USAGE
//   target_emulator.exe --scenario B [--port 1502] [--delay-ms 0]
//   then point Tech Aim at Modbus TCP 127.0.0.1:<port>.
//
// Scenario letters match docs/release/0.9.0-emulator-scenarios.md.
// ─────────────────────────────────────────────────────────────────────────────

#include <modbus.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#  include <winsock2.h>
#  define SLEEP_MS(ms) Sleep(ms)
#else
#  include <unistd.h>
#  define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

// The vendored libmodbus is NOT stock: QModMaster patched it to call these
// bus-monitor hooks on every frame, and modbus.c references them unconditionally.
// The application supplies them from modbusadapter.cpp; a standalone tool must
// provide its own or the link fails. Silent here - the emulator reports at the
// register level, which is the level the defects live at.
extern "C" void busMonitorRawRequestData(uint8_t*, uint8_t) {}
extern "C" void busMonitorRawResponseData(uint8_t*, uint8_t) {}

// The map must span the highest address the application touches. Shot 10 sits
// at 16376 + 80 = 16456, so 16600 registers of headroom is ample.
static const int kRegCount   = 16600;
static const int kRegHwCheck = 4096;
static const int kRegCount8192 = 8192;
static const int kRegReset   = 8193;
static const int kRegMotorOn = 8196;
static const int kRegMotorDur= 8197;
static const int kRegMotorTrg= 8198;
static const int kRegShotBase= 16376;

struct Emu {
    char scenario   = 'A';
    int  port       = 1502;
    int  delayMs    = 0;
    // The TARGET's own counter. Distinct from anything the app believes.
    int  hwCount    = 0;
    bool honourReset = true;   // scenario B sets this false
    int  resetsSeen = 0;
    int  motorRuns  = 0;
    bool motorOn    = false;
};

static void setShot(modbus_mapping_t* m, int index, double xMm, double yMm)
{
    // The application divides by 10 when is_single_decimal=1, and reads
    // [1] = x, [0] = y. Values are tenths of a millimetre.
    const int addr = kRegShotBase + 8 * index;
    if (addr + 1 >= kRegCount) return;
    m->tab_registers[addr + 1] = (uint16_t) (int) (xMm * 10.0);
    m->tab_registers[addr + 0] = (uint16_t) (int) (yMm * 10.0);
}

static void publishCount(modbus_mapping_t* m, int count)
{
    m->tab_registers[kRegCount8192 + 1] = (uint16_t) count;
    m->tab_registers[kRegCount8192 + 0] = 0;
}

static void applyScenario(Emu& e, modbus_mapping_t* m)
{
    // Hardware check must answer non-zero or the application refuses to start
    // a live match - exactly what happened when the target was powered off.
    m->tab_registers[kRegHwCheck + 0] = 2430;
    m->tab_registers[kRegHwCheck + 1] = 2430;

    switch (e.scenario) {
    case 'A':   // normal reset: counter starts at 5, reset works, then one shot
        e.hwCount = 5; e.honourReset = true;
        setShot(m, 1, 2.5, 2.9);
        break;

    case 'B':   // THE KEY TEST. Reset is acknowledged but the counter stays 1.
        // This is the RC2d safety path that hardware never gave us. The stale
        // coordinates are the RC2b shot that became a phantom on 2026-08-08.
        e.hwCount = 1; e.honourReset = false;
        setShot(m, 1, 2.5, 2.9);      // the residue that must NOT be replayed
        setShot(m, 2, -4.5, 5.2);     // the first REAL shot
        break;

    case 'C':   // stale coordinates, count never changes
        e.hwCount = 0; e.honourReset = true;
        setShot(m, 1, 9.9, 9.9);
        break;

    case 'D':   // baseline 4, advances to 5 => exactly one shot
        e.hwCount = 4; e.honourReset = false;
        setShot(m, 5, 1.1, -2.2);
        break;

    case 'E':   // duplicate polling: count returned repeatedly, never advances
        e.hwCount = 1; e.honourReset = false;
        setShot(m, 1, 0.5, 0.5);
        break;

    case 'R':   // RESTART-001: baseline 0 vs hardware 2, then 3.
        // The condition hardware would not reproduce on demand. The
        // application must SYNCHRONIZE to 2 and then accept the shot at 3 -
        // never reject in silence, and never replay the two residue counts as
        // shots. Slot 3 carries the coordinates of the shot RC2f actually lost.
        e.hwCount = 2; e.honourReset = false;
        setShot(m, 1, -4.2, -4.3);
        setShot(m, 2, -4.9, -0.3);
        setShot(m, 3, -5.4, -3.6);
        break;

    case 'X':   // counter goes BACKWARDS while acquiring
        e.hwCount = 5; e.honourReset = false;
        break;

    case 'I':   // slow firmware
        e.hwCount = 0; e.honourReset = true;
        if (e.delayMs == 0) e.delayMs = 200;
        break;

    default:
        e.hwCount = 0; e.honourReset = true;
        break;
    }
    publishCount(m, e.hwCount);
}

// Advance the emulated target by one shot, as if a pellet landed.
static void fireShot(Emu& e, modbus_mapping_t* m, double x, double y)
{
    ++e.hwCount;
    setShot(m, e.hwCount, x, y);
    publishCount(m, e.hwCount);
    printf("[emu] shot fired -> hardware counter now %d (x=%.1f y=%.1f)\n",
           e.hwCount, x, y);
    fflush(stdout);
}

int main(int argc, char* argv[])
{
    Emu e;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--scenario" && i + 1 < argc)      e.scenario = argv[++i][0];
        else if (a == "--port" && i + 1 < argc)     e.port = atoi(argv[++i]);
        else if (a == "--delay-ms" && i + 1 < argc) e.delayMs = atoi(argv[++i]);
        else if (a == "--help") {
            printf("target_emulator --scenario <A-N> [--port 1502] [--delay-ms 0]\n");
            return 0;
        }
    }

#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    modbus_mapping_t* map = modbus_mapping_new(0, 0, kRegCount, 0);
    if (!map) { fprintf(stderr, "[emu] mapping allocation failed\n"); return 1; }

    applyScenario(e, map);

    modbus_t* ctx = modbus_new_tcp("127.0.0.1", e.port);
    if (!ctx) { fprintf(stderr, "[emu] modbus_new_tcp failed\n"); return 1; }

    const int server = modbus_tcp_listen(ctx, 1);
    if (server == -1) {
        fprintf(stderr, "[emu] listen failed on port %d: %s\n", e.port, modbus_strerror(errno));
        return 1;
    }

    printf("=== Tech Aim target emulator ===\n");
    printf("scenario        : %c\n", e.scenario);
    printf("listening       : 127.0.0.1:%d (Modbus TCP)\n", e.port);
    printf("hardware counter: %d\n", e.hwCount);
    printf("honours reset   : %s\n", e.honourReset ? "yes" : "NO - acknowledges and ignores");
    printf("response delay  : %d ms\n", e.delayMs);
    printf("\nPoint Tech Aim at Modbus TCP 127.0.0.1:%d.\n", e.port);
    printf("Type 'f' + Enter to fire a shot, 'q' + Enter to quit.\n\n");
    fflush(stdout);

    modbus_tcp_accept(ctx, (int*) &server);

    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    for (;;) {
        const int rc = modbus_receive(ctx, query);
        if (rc == -1) break;               // client gone

        if (e.delayMs > 0) SLEEP_MS(e.delayMs);

        // Inspect the request BEFORE replying so writes can be intercepted.
        // Function 6 = write single register.
        if (rc >= 6 && query[7] == 0x06) {
            const int addr = (query[8] << 8) | query[9];
            const int val  = (query[10] << 8) | query[11];

            if (addr == kRegReset) {
                ++e.resetsSeen;
                if (e.honourReset) {
                    e.hwCount = 0;
                    publishCount(map, 0);
                    printf("[emu] reset #%d honoured -> counter 0\n", e.resetsSeen);
                } else {
                    // ACKNOWLEDGE but do nothing. modbus_reply below still
                    // returns success, so the application sees a clean write -
                    // precisely the field behaviour that produced a phantom.
                    printf("[emu] reset #%d ACKNOWLEDGED AND IGNORED -> counter stays %d\n",
                           e.resetsSeen, e.hwCount);
                }
                fflush(stdout);
            } else if (addr == kRegMotorTrg && val != 0) {
                ++e.motorRuns;
                e.motorOn = true;
                printf("[emu] MOTOR RUN #%d (paper feed)\n", e.motorRuns);
                fflush(stdout);
            } else if (addr == kRegMotorOn && val == 0) {
                e.motorOn = false;
            }
        }

        modbus_reply(ctx, query, rc, map);
    }

    printf("\n[emu] client disconnected. resets seen=%d, motor runs=%d, final counter=%d\n",
           e.resetsSeen, e.motorRuns, e.hwCount);
    modbus_mapping_free(map);
    modbus_close(ctx);
    modbus_free(ctx);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
