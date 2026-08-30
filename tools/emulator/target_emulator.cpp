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
#include <ctime>

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
    // Deterministic trigger. The emulator is single-client and single-threaded,
    // so a second Modbus connection cannot be used to advance the counter
    // without disturbing the application's own session - it was tried and timed
    // out. This fires once, N seconds after the first client connects.
    int  fireAfterSec = 0;
    bool fired        = false;
    // How far the counter moves when the trigger fires. 1 = a normal shot.
    // 2+ simulates shots missed while the application was not looking; a
    // negative value simulates a counter that went backwards (reset, power
    // cycle). Both must produce ACQUISITION FAULT, never a silent rejection.
    int  fireDelta = 1;
    double fireX = 0.3, fireY = 6.4;

    // ── ACQ-FLUSH-001 bench target ────────────────────────────────────────
    // The defect fired at every 10th shot, so a scenario that fires ONCE can
    // never reach it. This fires on a repeating interval, hands-free, for as
    // long as the operator leaves it running: 25 shots crosses two boundaries
    // and 60 crosses six.
    int  fireEverySec = 0;
    int  shotsFired   = 0;
    int  fireLimit    = 0;          // 0 = no limit
    // How long the TARGET takes to honour a counter reset. The RC2g build
    // assumed 2 600 ms and raced it; the point of this knob is that the
    // application must not care what the number is.
    int  resetLatencyMs = 0;
    bool resetPending   = false;
    time_t resetAskedAt = 0;

    // -- the remaining release-blocking scenarios (section 13) -------------
    int  jumpEverySec  = 0;        // H: counter leaps forward, no coordinates
    int  jumpBy        = 0;
    time_t lastJump    = 0;
    bool failCoordReads = false;   // J: the coordinate region answers an error
    int  flakyCoordOneIn = 0;      // K: every Nth coordinate read fails
    long coordReadSeq  = 0;
    int  dropAfterSec  = 0;        // L: drop the client, keep the counter
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

    case 'F':   // ACQ-FLUSH-001. The 10-shot boundary, hands-free.
        // Fires every 6 s from a clean counter and honours the reset after a
        // deliberate delay, so the application crosses a real series boundary
        // repeatedly with the target lagging. The RC2g build stopped
        // acquisition at the first one, 12 times out of 12.
        e.hwCount = 0; e.honourReset = true;
        if (e.fireEverySec == 0) e.fireEverySec = 6;
        if (e.resetLatencyMs == 0) e.resetLatencyMs = 500;
        break;

    case 'G':   // ACQ-DESYNC-002. The reconnect that produced repeated 10.8.
        // Ten shots are already in the slots and the counter reads 1: exactly
        // what Tablet-02 answered after the operator replugged the USB. The
        // application must reconcile or refuse - never number the next shot
        // past the end of its own arrays.
        e.hwCount = 1; e.honourReset = true;
        for (int i = 1; i <= 10; ++i) setShot(m, i, -0.5 + i * 0.3, 1.0 - i * 0.2);
        setShot(m, 1, -3.9, 1.0);      // the genuine next shot, as logged
        if (e.fireEverySec == 0) e.fireEverySec = 8;
        break;

    case 'H':   // Counter JUMPS forward with no coordinates behind it. A real
        // lost-shot condition - the guard must still fire. This is the case the
        // ACQ-FLUSH-001 fix must NOT have weakened.
        e.hwCount = 0; e.honourReset = true;
        if (e.jumpEverySec == 0) e.jumpEverySec = 12;
        if (e.jumpBy == 0) e.jumpBy = 4;
        break;

    case 'J':   // Every coordinate read fails. The counter still advances, so
        // the application sees a genuine shot and cannot read where it landed.
        // It must accept nothing rather than decode the buffer it was given.
        e.hwCount = 0; e.honourReset = true;
        e.failCoordReads = true;
        if (e.fireEverySec == 0) e.fireEverySec = 8;
        break;

    case 'K':   // A flaky link: one coordinate read in three fails. Partial
        // acquisition is worse than none, because some shots look fine.
        e.hwCount = 0; e.honourReset = true;
        e.flakyCoordOneIn = 3;
        if (e.fireEverySec == 0) e.fireEverySec = 8;
        break;

    case 'L':   // Disconnect mid-session and keep the counter. On reconnect the
        // target reports the SAME count it had - nothing was missed, and the
        // application must resume without inventing or replaying anything.
        e.hwCount = 0; e.honourReset = true;
        if (e.fireEverySec == 0) e.fireEverySec = 6;
        if (e.dropAfterSec == 0) e.dropAfterSec = 40;
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
    e.hwCount += e.fireDelta;
    if (e.hwCount < 0) e.hwCount = 0;
    if (e.hwCount > 0) setShot(m, e.hwCount, x, y);
    publishCount(m, e.hwCount);
    if (e.fireDelta == 1)
        printf("[emu] shot fired -> hardware counter now %d (x=%.1f y=%.1f)\n",
               e.hwCount, x, y);
    else
        printf("[emu] ANOMALY INJECTED: counter moved by %+d -> now %d\n",
               e.fireDelta, e.hwCount);
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
        else if (a == "--fire-after" && i + 1 < argc) e.fireAfterSec = atoi(argv[++i]);
        else if (a == "--fire-delta" && i + 1 < argc) e.fireDelta = atoi(argv[++i]);
        else if (a == "--fire-every" && i + 1 < argc) e.fireEverySec = atoi(argv[++i]);
        else if (a == "--fire-limit" && i + 1 < argc) e.fireLimit = atoi(argv[++i]);
        else if (a == "--reset-latency-ms" && i + 1 < argc) e.resetLatencyMs = atoi(argv[++i]);
        else if (a == "--jump-every" && i + 1 < argc) e.jumpEverySec = atoi(argv[++i]);
        else if (a == "--jump-by" && i + 1 < argc) e.jumpBy = atoi(argv[++i]);
        else if (a == "--drop-after" && i + 1 < argc) e.dropAfterSec = atoi(argv[++i]);
        else if (a == "--help") {
            printf("target_emulator --scenario <A-N> [--port 1502] [--delay-ms 0]\n"
                   "                [--fire-after SEC] [--fire-every SEC] [--fire-limit N]\n"
                   "                [--fire-delta N] [--reset-latency-ms MS]\n"
                   "  F  10-shot flush boundary, hands-free (ACQ-FLUSH-001)\n"
                   "  G  reconnect with a mismatched counter (ACQ-DESYNC-002)\n"
                   "  H  counter jumps forward with no coordinates (real lost shots)\n"
                   "  J  every coordinate read fails (ACQ-READ-004)\n"
                   "  K  one coordinate read in three fails - a flaky link\n"
                   "  L  disconnect mid-session, reconnect with the SAME counter\n");
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
    printf("reset latency   : %d ms\n", e.resetLatencyMs);
    if (e.fireEverySec > 0)
        printf("auto fire       : every %d s%s\n", e.fireEverySec,
               e.fireLimit > 0 ? " (limited)" : "");
    printf("\nPoint Tech Aim at Modbus TCP 127.0.0.1:%d.\n", e.port);
    printf("Type 'f' + Enter to fire a shot, 'q' + Enter to quit.\n\n");
    fflush(stdout);

    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

    // Accept clients REPEATEDLY. The first version exited when its one client
    // disconnected, so closing and relaunching the application killed the
    // emulator and every later connection attempt failed with no explanation.
    // The application must be able to restart, and reconnect, against a target
    // that is still there - which is the whole point of several scenarios.
    int serverFd = server;
    for (;;) {
      if (modbus_tcp_accept(ctx, &serverFd) == -1) {
          fprintf(stderr, "[emu] accept failed: %s\n", modbus_strerror(errno));
          break;
      }
      printf("[emu] client connected (counter=%d)\n", e.hwCount);
      if (e.fireAfterSec > 0 && !e.fired)
          printf("[emu] will fire ONE shot %d s from now\n", e.fireAfterSec);
      fflush(stdout);
      const time_t connectedAt = time(NULL);
      time_t lastAutoFire = connectedAt;

      for (;;) {
        const int rc = modbus_receive(ctx, query);
        if (rc == -1) {
            printf("[emu] client disconnected - still listening, counter stays %d\n",
                   e.hwCount);
            fflush(stdout);
            break;                     // back to accept, do NOT exit
        }

        if (e.delayMs > 0) SLEEP_MS(e.delayMs);

        // Deterministic single shot, once the countdown expires.
        if (e.fireAfterSec > 0 && !e.fired
            && difftime(time(NULL), connectedAt) >= e.fireAfterSec) {
            e.fired = true;
            fireShot(e, map, e.fireX, e.fireY);
        }

        // Repeating hands-free fire. Coordinates walk so no two shots share
        // one - a frozen coordinate is then obvious in the application.
        if (e.fireEverySec > 0
            && (e.fireLimit == 0 || e.shotsFired < e.fireLimit)
            && difftime(time(NULL), lastAutoFire) >= e.fireEverySec) {
            lastAutoFire = time(NULL);
            ++e.shotsFired;
            fireShot(e, map,
                     -8.0 + (e.shotsFired % 17) * 1.0,
                      7.0 - (e.shotsFired % 13) * 1.1);
        }

        // H. A counter that leaps forward with NOTHING written behind it -
        // shots the target counted and the application never got. This is a
        // real lost-shot condition and the guard must still catch it: the
        // ACQ-FLUSH-001 work must not have bought its boundary by going deaf.
        if (e.jumpEverySec > 0
            && difftime(time(NULL), e.lastJump) >= e.jumpEverySec) {
            e.lastJump = time(NULL);
            e.hwCount += e.jumpBy;
            publishCount(map, e.hwCount);
            printf("[emu] COUNTER JUMPED by %d with no coordinates -> now %d\n",
                   e.jumpBy, e.hwCount);
            fflush(stdout);
        }

        // L. Drop the client mid-session and keep the counter. The application
        // must come back, see the SAME count, and resume without replaying.
        if (e.dropAfterSec > 0
            && difftime(time(NULL), connectedAt) >= e.dropAfterSec) {
            e.dropAfterSec = 0;                  // once per run
            printf("[emu] DROPPING THE CLIENT - counter stays %d\n", e.hwCount);
            fflush(stdout);
            break;
        }

        // A reset the target honours only after resetLatencyMs. The
        // application must judge nothing while its own reset is outstanding.
        if (e.resetPending && difftime(time(NULL), e.resetAskedAt) * 1000.0
                                  >= e.resetLatencyMs) {
            e.resetPending = false;
            e.hwCount = 0;
            publishCount(map, e.hwCount);
            printf("[emu] reset honoured after %d ms - counter now 0\n",
                   e.resetLatencyMs);
            fflush(stdout);
        }

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

        // J / K. ACQ-READ-004 at the wire. A read of the coordinate region is
        // answered with a server failure instead of data, so the application
        // gets a negative return code and a buffer it did not fill. It must
        // decode nothing, accept nothing, score nothing and feed nothing.
        if (rc >= 6 && (query[7] == 0x03 || query[7] == 0x04)) {
            const int raddr = (query[8] << 8) | query[9];
            if (raddr > kRegShotBase) {
                ++e.coordReadSeq;
                const bool fail = e.failCoordReads
                    || (e.flakyCoordOneIn > 0 && (e.coordReadSeq % e.flakyCoordOneIn) == 0);
                if (fail) {
                    printf("[emu] COORDINATE READ REFUSED addr=%d (seq %ld)\n",
                           raddr, e.coordReadSeq);
                    fflush(stdout);
                    modbus_reply_exception(ctx, query,
                                           MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE);
                    continue;
                }
            }
        }

        modbus_reply(ctx, query, rc, map);

        // modbus_reply() APPLIES write requests to the mapping itself, so
        // inspecting the query above is not enough to suppress one. And
        // register 8193 IS the counter cell - publishCount() writes the count
        // to 8192+1 = 8193, which is exactly the register the application
        // writes to "reset". The reset is therefore a direct write of the
        // counter to zero, on the emulator and on the real target alike.
        //
        // Scenario B/R must hold a NON-ZERO counter across an acknowledged
        // reset, so the value is restored AFTER the reply. The application
        // still sees a successful write; the counter simply does not move.
        if (rc >= 6 && query[7] == 0x06) {
            const int addr = (query[8] << 8) | query[9];
            if (addr == kRegReset || addr == kRegCount8192 + 1) {
                if (!e.honourReset) {
                    publishCount(map, e.hwCount);
                    printf("[emu] RESET REQUEST RECEIVED (fc=6 addr=%d)\n"
                           "[emu] RESET ACKNOWLEDGED\n"
                           "[emu] RESET INTENTIONALLY IGNORED\n"
                           "[emu] COUNTER REMAINS %d\n", addr, e.hwCount);
                    fflush(stdout);
                } else if (e.resetLatencyMs > 0 && !e.resetPending) {
                    // Acknowledged now, honoured later - the condition the
                    // application used to race against and lose.
                    e.resetPending = true;
                    e.resetAskedAt = time(NULL);
                    publishCount(map, e.hwCount);
                    printf("[emu] RESET REQUEST RECEIVED - will honour it in %d ms; "
                           "counter stays %d until then\n", e.resetLatencyMs, e.hwCount);
                    fflush(stdout);
                }
            }
        }
      }
    }

    printf("\n[emu] shutting down. resets seen=%d, motor runs=%d, final counter=%d\n",
           e.resetsSeen, e.motorRuns, e.hwCount);
    modbus_mapping_free(map);
    modbus_close(ctx);
    modbus_free(ctx);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
