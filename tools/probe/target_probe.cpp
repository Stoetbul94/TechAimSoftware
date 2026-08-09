// Tech Aim — read-only target probe.
//
// Answers one question that no log currently records: what does the target's
// own shot counter actually say? RESTART-001 has two equally consistent
// explanations - application baseline BEHIND the target, or AHEAD of it - and
// they need opposite fixes. This measures instead of inferring.
//
// READ ONLY. It writes no register, resets nothing, moves no motor.
//
//   target_probe.exe [COM4] [19200]
#include <modbus.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" void busMonitorRawRequestData(uint8_t*, uint8_t) {}
extern "C" void busMonitorRawResponseData(uint8_t*, uint8_t) {}

int main(int argc, char* argv[])
{
    const char* port = (argc > 1) ? argv[1] : "COM4";
    const int   baud = (argc > 2) ? atoi(argv[2]) : 19200;

    // Same line settings the application logs in the field:
    // "portname COM4 19200 Even 8 1 Disable 1"
    modbus_t* ctx = modbus_new_rtu(port, baud, 'E', 8, 1, 0);   // last arg = RTS mode, Disable
    if (!ctx) { fprintf(stderr, "modbus_new_rtu failed\n"); return 1; }
    modbus_set_slave(ctx, 1);
    modbus_set_response_timeout(ctx, 1, 0);

    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "connect to %s failed: %s\n", port, modbus_strerror(errno));
        fprintf(stderr, "(is Tech Aim still running and holding the port?)\n");
        modbus_free(ctx);
        return 1;
    }

    uint16_t r[8];
    printf("=== Tech Aim target probe (READ ONLY) ===\nport: %s @ %d 8E1\n\n", port, baud);

    memset(r, 0, sizeof(r));
    if (modbus_read_registers(ctx, 8192, 2, r) >= 0)
        printf("  8192 shot counter   : [0]=%u  [1]=%u   <-- [1] IS THE COUNTER\n", r[0], r[1]);
    else
        printf("  8192 shot counter   : READ FAILED (%s)\n", modbus_strerror(errno));

    memset(r, 0, sizeof(r));
    if (modbus_read_registers(ctx, 4096, 2, r) >= 0)
        printf("  4096 hardware check : [0]=%u  [1]=%u   (non-zero = alive)\n", r[0], r[1]);
    else
        printf("  4096 hardware check : READ FAILED (%s)\n", modbus_strerror(errno));

    // Coordinates for the first few shot slots, so a residue count can be
    // matched against the shots it belongs to.
    for (int i = 1; i <= 4; ++i) {
        memset(r, 0, sizeof(r));
        if (modbus_read_registers(ctx, 16376 + 8 * i, 2, r) >= 0)
            printf("  shot %d coords       : x=%.1f  y=%.1f\n",
                   i, (int16_t) r[1] / 10.0, (int16_t) r[0] / 10.0);
    }

    modbus_close(ctx);
    modbus_free(ctx);
    return 0;
}
