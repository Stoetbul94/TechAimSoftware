// JNI half of the Android USB transport.
//
// This file is the ONLY place that knows Java exists. It implements the
// IUsbDeviceBridge interface the state machine already depends on, so
// AndroidUsbTransport never sees a JNI type and stays testable with a scripted
// bridge on any platform.
//
// WHAT CROSSES THE BOUNDARY. Connection state, attach/detach, permission
// result, bytes, errors, and the selected VID/PID. Nothing else. No shot, no
// score, no competition concept crosses this line in either direction - if one
// ever needs to, the design has gone wrong somewhere above.
//
// The whole file compiles to nothing on desktop, so the desktop build is
// untouched by its existence.

#include "target/AndroidUsbTransport.h"

#if defined(Q_OS_ANDROID)

#include <QCoreApplication>
#include <QDebug>
#include <QJniEnvironment>
#include <QJniObject>
#include <QPointer>
#include <QtCore/qcoreapplication_platform.h>

namespace ta {

namespace {
const char* kBridgeClass = "za/co/techaim/target/UsbSerialBridge";
}

// The transport the Java callbacks advance. A QPointer so a callback that
// arrives after Qt has torn the object down finds null instead of freed
// memory - the shutdown race §16 asks about.
//
// NOT in the anonymous namespace: the extern "C" callbacks at the bottom of
// this file name it as ta::g_transport, which an unnamed namespace would not
// give them.
QPointer<AndroidUsbTransport> g_transport;

void setAndroidUsbCallbackTarget(AndroidUsbTransport* t) { g_transport = t; }

class AndroidUsbBridge : public IUsbDeviceBridge
{
public:
    QList<UsbDeviceId> enumerate() override
    {
        QList<UsbDeviceId> out;
        QJniEnvironment env;
        QJniObject arr = QJniObject::callStaticObjectMethod(
            kBridgeClass, "enumerateSupported", "()[I");
        if (!arr.isValid())
            return out;
        jintArray ja = arr.object<jintArray>();
        const jsize n = env->GetArrayLength(ja);
        QVector<jint> buf(n);
        env->GetIntArrayRegion(ja, 0, n, buf.data());
        // Flat [vid,pid, vid,pid, ...] - a trivial JNI signature beats a
        // parcelable for six integers.
        for (jsize i = 0; i + 1 < n; i += 2)
            out.append(UsbDeviceId{ int(buf[i]), int(buf[i + 1]) });
        return out;
    }

    bool hasPermission(const UsbDeviceId& id) override
    {
        return QJniObject::callStaticMethod<jboolean>(
            kBridgeClass, "hasPermission", "(II)Z", id.vendorId, id.productId);
    }

    bool requestPermission(const UsbDeviceId& id) override
    {
        return QJniObject::callStaticMethod<jboolean>(
            kBridgeClass, "requestPermission", "(II)Z", id.vendorId, id.productId);
    }

    bool open(const UsbDeviceId& id, const SerialLineConfig& cfg) override
    {
        // usb-serial-for-android parity constants: 0 none, 1 odd, 2 EVEN.
        // The field value is EVEN; mapping it wrong would produce a port that
        // opens and returns framing errors, which is worse than a port that
        // refuses to open.
        jint parity = 2;
        switch (cfg.parity) {
        case 'N': case 'n': parity = 0; break;
        case 'O': case 'o': parity = 1; break;
        case 'E': case 'e': default: parity = 2; break;
        }
        return QJniObject::callStaticMethod<jboolean>(
            kBridgeClass, "open", "(IIIIIIZ)Z",
            jint(id.vendorId), jint(id.productId), jint(cfg.baud),
            jint(cfg.dataBits), jint(cfg.stopBits), parity, jboolean(cfg.rts));
    }

    void close() override
    {
        QJniObject::callStaticMethod<void>(kBridgeClass, "close", "()V");
    }

    int write(const QByteArray& data) override
    {
        QJniEnvironment env;
        jbyteArray ja = env->NewByteArray(data.size());
        env->SetByteArrayRegion(ja, 0, data.size(),
                                reinterpret_cast<const jbyte*>(data.constData()));
        const jint n = QJniObject::callStaticMethod<jint>(
            kBridgeClass, "write", "([BI)I", ja, jint(200));
        env->DeleteLocalRef(ja);
        return int(n);
    }

    int read(QByteArray* out, int maxBytes, int /*timeoutMs*/) override
    {
        QJniEnvironment env;
        QJniObject arr = QJniObject::callStaticObjectMethod(
            kBridgeClass, "read", "(I)[B", jint(maxBytes));
        // Java returns NULL for failure and an EMPTY array for "nothing yet".
        // Collapsing those two would turn a dead link into a quiet idle link,
        // which is exactly the class of defect ACQ-READ-004 exists to stop.
        if (!arr.isValid())
            return -1;
        jbyteArray ja = arr.object<jbyteArray>();
        const jsize n = env->GetArrayLength(ja);
        if (n > 0 && out) {
            QByteArray chunk(n, Qt::Uninitialized);
            env->GetByteArrayRegion(ja, 0, n, reinterpret_cast<jbyte*>(chunk.data()));
            out->append(chunk);
        }
        return int(n);
    }

    QString lastError() const override
    {
        // Explicit signature rather than the templated overload: Qt 6.5's
        // qjnitypes deduction rejects a const char* class name held in a
        // variable, and one named constant is better than a literal repeated
        // at every call site.
        return QJniObject::callStaticObjectMethod(
                   kBridgeClass, "lastError", "()Ljava/lang/String;").toString();
    }
};

IUsbDeviceBridge* androidUsbBridge()
{
    static AndroidUsbBridge s_bridge;
    return &s_bridge;
}

// Hands Java the Activity it needs for getSystemService() and for registering
// the attach/detach/permission receivers. Called once at startup; without it
// the Java side has no Context and every enumeration returns empty - which
// would look exactly like "no target attached".
void initialiseAndroidUsb(AndroidUsbTransport* transport)
{
    setAndroidUsbCallbackTarget(transport);
    // Qt 6.5: context() returns QtJniTypes::Context, which wraps a jobject.
    QJniObject activity(QNativeInterface::QAndroidApplication::context());
    if (!activity.isValid()) {
        qWarning("Android USB: no activity context; USB transport unavailable");
        return;
    }
    QJniObject::callStaticMethod<void>(
        kBridgeClass, "setActivity", "(Landroid/app/Activity;)V",
        activity.object<jobject>());
    if (transport)
        transport->setBridge(androidUsbBridge());
}

bool androidHasUsbHostFeature()
{
    return QJniObject::callStaticMethod<jboolean>(
        kBridgeClass, "hasUsbHostFeature", "()Z");
}

QString androidUsbProductName(const UsbDeviceId& id)
{
    return QJniObject::callStaticObjectMethod(
               kBridgeClass, "deviceProductName", "(II)Ljava/lang/String;",
               id.vendorId, id.productId).toString();
}

} // namespace ta

// ── the three callbacks Java invokes ──────────────────────────────────────
// Each one checks the QPointer first. A permission result or a detach that
// lands after shutdown must be a no-op, not a call into freed memory.
extern "C" {

JNIEXPORT void JNICALL
Java_za_co_techaim_target_UsbSerialBridge_nativeDeviceAttached(JNIEnv*, jclass)
{
    if (ta::g_transport) ta::g_transport->onDeviceAttached();
}

JNIEXPORT void JNICALL
Java_za_co_techaim_target_UsbSerialBridge_nativeDeviceDetached(JNIEnv*, jclass)
{
    if (ta::g_transport) ta::g_transport->onDeviceDetached();
}

JNIEXPORT void JNICALL
Java_za_co_techaim_target_UsbSerialBridge_nativePermissionResult(JNIEnv*, jclass,
                                                                 jboolean granted)
{
    if (ta::g_transport) ta::g_transport->onPermissionResult(granted == JNI_TRUE);
}

} // extern "C"

#endif // Q_OS_ANDROID
