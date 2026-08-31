// libmodbus backend for the Android USB transport.
//
// THE POINT OF THIS FILE IS HOW LITTLE IS IN IT.
//
// libmodbus is built around a backend vtable. Everything that defines the
// PROTOCOL - request framing, response framing, CRC, slave addressing - stays
// with the RTU backend and is reused by copying its function pointers
// verbatim. Only the members that move BYTES are replaced:
//
//     connect, close, flush, send, recv, select   (+ receive and free, below)
//
// So there is no second Modbus RTU implementation, no re-implemented CRC, and
// no framing anywhere in Java. A framing bug cannot be introduced here because
// no framing code is written here.
//
// WHY receive AND free ARE ALSO OVERRIDDEN. They are the only two retained RTU
// functions that dereference ctx->backend_data, and our backend_data is not a
// modbus_rtu_t. Verified by inspection of modbus-rtu.c: set_slave,
// build_request_basis, build_response_basis, prepare_response_tid,
// send_msg_pre, check_integrity and pre_check_confirmation touch it ZERO
// times, which is what makes reusing them safe rather than hopeful.
//
// _modbus_rtu_receive's use of backend_data is the RS-485 echo suppression
// flag (confirmation_to_ignore) that matters when libmodbus acts as a SERVER
// on a line where the transmitter hears itself. Tech Aim is a master over a
// point-to-point USB link with RTS deasserted, so the replacement is the plain
// indication receive with no echo bookkeeping.

#include "target/AndroidUsbTransport.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QThread>

extern "C" {
#include "modbus.h"
#include "modbus-private.h"
#include "modbus-rtu.h"
// Declared here rather than in a lambda: it is defined in modbus-rtu.c, which
// is compiled as C, so it needs C linkage or the copy below will not link.
extern const modbus_backend_t _modbus_rtu_backend;
}

#include <cerrno>
#include <cstring>

namespace ta {

namespace {

// Our backend_data. Deliberately tiny: the transport does the work.
struct AndroidUsbBackendData {
    AndroidUsbTransport* transport = nullptr;
    QByteArray           pending;      // bytes read from Java, not yet consumed
};

inline AndroidUsbBackendData* data(modbus_t* ctx)
{
    return static_cast<AndroidUsbBackendData*>(ctx->backend_data);
}

int ta_connect(modbus_t* ctx)
{
    AndroidUsbBackendData* d = data(ctx);
    if (!d || !d->transport || !d->transport->isReady()) {
        errno = ECONNREFUSED;
        return -1;
    }
    // libmodbus keeps a descriptor in ctx->s and checks it for >= 0 in places.
    // There is no real fd here; a non-negative sentinel keeps those checks
    // honest without pretending to be a file.
    ctx->s = 1;
    return 0;
}

void ta_close(modbus_t* ctx)
{
    AndroidUsbBackendData* d = data(ctx);
    if (d) d->pending.clear();
    ctx->s = -1;
}

int ta_flush(modbus_t* ctx)
{
    AndroidUsbBackendData* d = data(ctx);
    if (!d) return -1;
    // ACQ-FLUSH-001 territory: a flush must actually discard, on BOTH sides of
    // the boundary. Dropping our local pending bytes while leaving a backlog
    // queued in Java would let a stale frame arrive after the flush.
    d->pending.clear();
    if (d->transport) {
        QByteArray sink;
        while (d->transport->readBytes(&sink, 4096, 0) > 0)
            sink.clear();
    }
    return 0;
}

ssize_t ta_send(modbus_t* ctx, const uint8_t* req, int req_length)
{
    AndroidUsbBackendData* d = data(ctx);
    if (!d || !d->transport) { errno = EIO; return -1; }
    const int n = d->transport->writeBytes(
        QByteArray(reinterpret_cast<const char*>(req), req_length));
    if (n < 0) {
        // §15: a failed write is reported as a failure. It never returns the
        // length it wished it had sent.
        errno = EIO;
        return -1;
    }
    return n;
}

ssize_t ta_recv(modbus_t* ctx, uint8_t* rsp, int rsp_length)
{
    AndroidUsbBackendData* d = data(ctx);
    if (!d || !d->transport) { errno = EIO; return -1; }

    if (d->pending.isEmpty()) {
        QByteArray in;
        const int n = d->transport->readBytes(&in, rsp_length, 0);
        if (n < 0) {
            // §14 / ACQ-READ-004. A transport failure stays a failure all the
            // way up. It is never softened into "0 bytes available", because
            // libmodbus would then wait for a frame that is never coming and
            // report a timeout instead of a broken link - and the operator
            // would be told the wrong thing.
            errno = EIO;
            return -1;
        }
        d->pending = in;
    }

    if (d->pending.isEmpty())
        return 0;

    const int take = qMin(rsp_length, int(d->pending.size()));
    memcpy(rsp, d->pending.constData(), size_t(take));
    d->pending.remove(0, take);
    return take;
}

int ta_select(modbus_t* ctx, fd_set* /*rset*/, struct timeval* tv,
              int /*length_to_read*/)
{
    AndroidUsbBackendData* d = data(ctx);
    if (!d || !d->transport) { errno = EIO; return -1; }

    int budgetMs = 0;
    if (tv) budgetMs = int(tv->tv_sec * 1000 + tv->tv_usec / 1000);

    QElapsedTimer clock;
    clock.start();
    for (;;) {
        if (!d->pending.isEmpty())
            return 1;
        QByteArray in;
        const int n = d->transport->readBytes(&in, 4096, 0);
        if (n < 0) { errno = EIO; return -1; }
        if (n > 0) { d->pending.append(in); return 1; }
        if (clock.elapsed() >= budgetMs) {
            errno = ETIMEDOUT;      // libmodbus's own convention for a timeout
            return -1;
        }
        // A small sleep rather than a spin: §15 forbids hammering the device
        // in a tight loop, and this is the loop that would do it.
        QThread::msleep(2);
    }
}

int ta_receive(modbus_t* ctx, uint8_t* req)
{
    // No RS-485 echo bookkeeping - see the file header.
    return _modbus_receive_msg(ctx, req, MSG_INDICATION);
}

void ta_free(modbus_t* ctx)
{
    delete data(ctx);
    ctx->backend_data = nullptr;
    free(ctx);
}

// The vtable: a COPY of the RTU one with six-plus-two members replaced. Built
// once, lazily, so the copy happens after _modbus_rtu_backend is initialised.
const modbus_backend_t* androidUsbBackend()
{
    static modbus_backend_t s_backend = [] {
        modbus_backend_t b = _modbus_rtu_backend;   // framing + CRC, verbatim
        b.connect = ta_connect;
        b.close   = ta_close;
        b.flush   = ta_flush;
        b.send    = ta_send;
        b.recv    = ta_recv;
        b.select  = ta_select;
        b.receive = ta_receive;   // backend_data-coupled in RTU
        b.free    = ta_free;      // backend_data-coupled in RTU
        return b;
    }();
    return &s_backend;
}

} // namespace

modbus_t* modbus_new_android_usb(AndroidUsbTransport* transport)
{
    if (!transport)
        return nullptr;

    modbus_t* ctx = static_cast<modbus_t*>(malloc(sizeof(modbus_t)));
    if (!ctx)
        return nullptr;
    memset(ctx, 0, sizeof(modbus_t));
    _modbus_init_common(ctx);

    AndroidUsbBackendData* d = new AndroidUsbBackendData;
    d->transport = transport;

    ctx->backend      = androidUsbBackend();
    ctx->backend_data = d;
    ctx->s            = -1;
    return ctx;
}

} // namespace ta
