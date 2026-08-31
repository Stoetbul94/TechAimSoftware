package za.co.techaim.target;

// Tech Aim — Android USB target transport (Java half).
//
// SCOPE. This class moves BYTES and reports CONNECTION STATE. That is all it
// is permitted to do. There is deliberately no scoring, no shot acceptance, no
// counter reconciliation, no paper-feed policy and no competition state here:
// those live in the shared C++ core that both platforms use, and duplicating
// any of them in Java would create a second authority that could disagree with
// the first. If a future change wants to answer "was that a real shot?" in
// this file, the answer belongs somewhere else.
//
// The chip work is done by usb-serial-for-android (MIT, vendored as an AAR).
// We do not write CH340 control transfers by hand - see
// docs/android/ANDROID-USB-TRANSPORT-IMPLEMENTATION.md for why the four
// candidate paths were investigated and this one chosen.
//
// THREADING. Reads happen on a dedicated reader thread and never on the UI
// thread. The native side is notified through synchronized state; the byte
// queue is the only shared mutable data and it is guarded.

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.util.Log;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import java.io.IOException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;

public class UsbSerialBridge
{
    private static final String TAG = "TechAimUsb";
    private static final String ACTION_PERMISSION =
        "za.co.techaim.target.USB_PERMISSION";

    // The ONE place the accepted chip identity lives on the Java side. It
    // mirrors AndroidUsbTransport.cpp, and both were taken from the vendored
    // driver's own getSupportedDevices() rather than from memory.
    private static final int CH340_VID  = 0x1A86;
    private static final int CH340_PID  = 0x7523;
    private static final int CH341_PID  = 0x5523;

    private static Activity s_activity;
    private static UsbSerialBridge s_instance;

    private UsbManager m_usbManager;
    private UsbSerialPort m_port;
    private UsbDeviceConnection m_connection;
    private Thread m_reader;
    private volatile boolean m_readerRunning;

    private final ArrayDeque<byte[]> m_rx = new ArrayDeque<>();
    private final Object m_rxLock = new Object();
    private int m_rxBytes;

    private volatile String m_lastError = "";
    private volatile int m_vid, m_pid;
    private volatile boolean m_open;

    // Called from Qt's activity. Kept static because the native side has no
    // Activity handle of its own.
    public static void setActivity(Activity activity)
    {
        s_activity = activity;
        if (s_instance == null)
            s_instance = new UsbSerialBridge();
        s_instance.attachReceivers();
    }

    private static UsbSerialBridge inst()
    {
        if (s_instance == null)
            s_instance = new UsbSerialBridge();
        return s_instance;
    }

    // ── native callbacks (implemented in the JNI bridge) ──────────────────
    public static native void nativeDeviceAttached();
    public static native void nativeDeviceDetached();
    public static native void nativePermissionResult(boolean granted);

    private final BroadcastReceiver m_receiver = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (ACTION_PERMISSION.equals(action)) {
                final boolean granted = intent.getBooleanExtra(
                    UsbManager.EXTRA_PERMISSION_GRANTED, false);
                // The ANSWER, not the request, is what advances the state
                // machine. Treating requestPermission() as blocking is how
                // these flows deadlock.
                safeNative(new Runnable() { public void run() {
                    nativePermissionResult(granted); } });
            } else if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                safeNative(new Runnable() { public void run() {
                    nativeDeviceAttached(); } });
            } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                // Close FIRST, then tell the native side. If the order were
                // reversed the reader thread could still be holding a
                // connection to a device that has physically gone.
                inst().closePort();
                safeNative(new Runnable() { public void run() {
                    nativeDeviceDetached(); } });
            }
        }
    };

    // A callback arriving after Qt has torn down would call into freed
    // objects. Swallowing it here is the difference between a clean exit and a
    // crash report the tester cannot explain.
    private static void safeNative(Runnable r)
    {
        try { r.run(); }
        catch (UnsatisfiedLinkError e) { Log.w(TAG, "native not ready: " + e); }
        catch (Throwable t) { Log.w(TAG, "native callback failed: " + t); }
    }

    private void attachReceivers()
    {
        if (s_activity == null) return;
        m_usbManager = (UsbManager) s_activity.getSystemService(Context.USB_SERVICE);
        IntentFilter f = new IntentFilter();
        f.addAction(ACTION_PERMISSION);
        f.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        f.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        try {
            if (Build.VERSION.SDK_INT >= 33)
                s_activity.registerReceiver(m_receiver, f, Context.RECEIVER_NOT_EXPORTED);
            else
                s_activity.registerReceiver(m_receiver, f);
        } catch (Throwable t) {
            Log.w(TAG, "receiver registration failed: " + t);
        }
    }

    private static boolean supported(UsbDevice d)
    {
        // A whitelist, not "anything with bulk endpoints". A phone, a printer
        // or a debug adapter must never be adopted as a scoring target.
        return d.getVendorId() == CH340_VID
            && (d.getProductId() == CH340_PID || d.getProductId() == CH341_PID);
    }

    // ── enumeration ───────────────────────────────────────────────────────
    // Returns [vid,pid, vid,pid, ...] for SUPPORTED devices only. A flat int
    // array keeps the JNI signature trivial.
    public static int[] enumerateSupported()
    {
        UsbSerialBridge b = inst();
        List<Integer> out = new ArrayList<>();
        try {
            if (b.m_usbManager == null && s_activity != null)
                b.m_usbManager = (UsbManager) s_activity.getSystemService(Context.USB_SERVICE);
            if (b.m_usbManager == null) return new int[0];
            for (UsbDevice d : b.m_usbManager.getDeviceList().values()) {
                if (supported(d)) { out.add(d.getVendorId()); out.add(d.getProductId()); }
            }
        } catch (Throwable t) {
            b.m_lastError = "enumeration failed: " + t;
            return new int[0];
        }
        int[] arr = new int[out.size()];
        for (int i = 0; i < out.size(); ++i) arr[i] = out.get(i);
        return arr;
    }

    private UsbDevice findDevice(int vid, int pid)
    {
        if (m_usbManager == null) return null;
        for (UsbDevice d : m_usbManager.getDeviceList().values())
            if (d.getVendorId() == vid && d.getProductId() == pid) return d;
        return null;
    }

    public static boolean hasPermission(int vid, int pid)
    {
        UsbSerialBridge b = inst();
        UsbDevice d = b.findDevice(vid, pid);
        return d != null && b.m_usbManager != null && b.m_usbManager.hasPermission(d);
    }

    public static boolean requestPermission(int vid, int pid)
    {
        UsbSerialBridge b = inst();
        try {
            UsbDevice d = b.findDevice(vid, pid);
            if (d == null || s_activity == null) return false;
            int flags = 0;
            if (Build.VERSION.SDK_INT >= 31) flags = 0x02000000; // FLAG_MUTABLE
            PendingIntent pi = PendingIntent.getBroadcast(
                s_activity, 0, new Intent(ACTION_PERMISSION), flags);
            b.m_usbManager.requestPermission(d, pi);
            return true;
        } catch (Throwable t) {
            b.m_lastError = "permission request failed: " + t;
            return false;
        }
    }

    // ── open / configure ──────────────────────────────────────────────────
    // Opens AND configures. Both or neither: a port that opened but could not
    // be set to 19200/Even/8/1 would return bytes, and they would be wrong.
    public static boolean open(int vid, int pid, int baud, int dataBits,
                               int stopBits, int parity, boolean rts)
    {
        UsbSerialBridge b = inst();
        b.closePort();
        try {
            UsbDevice dev = b.findDevice(vid, pid);
            if (dev == null) { b.m_lastError = "device not present"; return false; }
            if (!b.m_usbManager.hasPermission(dev)) {
                b.m_lastError = "no USB permission"; return false;
            }
            List<UsbSerialDriver> drivers =
                UsbSerialProber.getDefaultProber().findAllDrivers(b.m_usbManager);
            UsbSerialDriver driver = null;
            for (UsbSerialDriver d : drivers)
                if (d.getDevice().getVendorId() == vid
                    && d.getDevice().getProductId() == pid) { driver = d; break; }
            if (driver == null) { b.m_lastError = "no driver for device"; return false; }

            b.m_connection = b.m_usbManager.openDevice(driver.getDevice());
            if (b.m_connection == null) { b.m_lastError = "openDevice returned null"; return false; }

            b.m_port = driver.getPorts().get(0);
            b.m_port.open(b.m_connection);
            b.m_port.setParameters(baud, dataBits, stopBits, parity);

            // RTS is set explicitly because the field log records it Disabled.
            // DTR is deliberately NOT touched: the desktop application never
            // manages it, so asserting a value here would be inventing
            // behaviour this project has never observed. Some boards wire DTR
            // to reset - the physical checklist watches for exactly that.
            try { b.m_port.setRTS(rts); }
            catch (Throwable t) { Log.w(TAG, "setRTS unsupported: " + t); }

            b.m_open = true;
            b.m_vid = vid; b.m_pid = pid; b.m_lastError = "";
            b.startReader();
            return true;
        } catch (Throwable t) {
            b.m_lastError = "open failed: " + t;
            b.closePort();
            return false;
        }
    }

    private void startReader()
    {
        m_readerRunning = true;
        m_reader = new Thread(new Runnable() {
            @Override public void run() {
                final byte[] buf = new byte[4096];
                while (m_readerRunning) {
                    try {
                        UsbSerialPort p = m_port;
                        if (p == null) break;
                        int n = p.read(buf, 200);
                        if (n > 0) {
                            byte[] chunk = new byte[n];
                            System.arraycopy(buf, 0, chunk, 0, n);
                            synchronized (m_rxLock) { m_rx.addLast(chunk); m_rxBytes += n; }
                        }
                    } catch (Throwable t) {
                        // A read failure is a TRANSPORT failure. It is recorded
                        // and the loop ends; it is never turned into bytes.
                        m_lastError = "read failed: " + t;
                        m_readerRunning = false;
                        break;
                    }
                }
            }
        }, "TechAimUsbReader");
        m_reader.setDaemon(true);
        m_reader.start();
    }

    public static void close() { inst().closePort(); }

    private void closePort()
    {
        m_readerRunning = false;
        m_open = false;
        Thread r = m_reader; m_reader = null;
        if (r != null) { try { r.join(500); } catch (InterruptedException ignored) {} }
        UsbSerialPort p = m_port; m_port = null;
        if (p != null) { try { p.close(); } catch (IOException ignored) {} }
        UsbDeviceConnection c = m_connection; m_connection = null;
        if (c != null) { try { c.close(); } catch (Throwable ignored) {} }
        synchronized (m_rxLock) { m_rx.clear(); m_rxBytes = 0; }
    }

    // ── byte movement ─────────────────────────────────────────────────────
    // Negative is FAILURE and stays failure. It is never softened to 0, which
    // would read as "nothing to send/receive" and hide a dead link.
    public static int write(byte[] data, int timeoutMs)
    {
        UsbSerialBridge b = inst();
        try {
            UsbSerialPort p = b.m_port;
            if (p == null || !b.m_open) { b.m_lastError = "not open"; return -1; }
            p.write(data, timeoutMs);
            return data.length;
        } catch (Throwable t) {
            b.m_lastError = "write failed: " + t;
            return -1;
        }
    }

    public static byte[] read(int maxBytes)
    {
        UsbSerialBridge b = inst();
        if (!b.m_open) return null;             // null == failure, not "empty"
        synchronized (b.m_rxLock) {
            if (b.m_rx.isEmpty()) return new byte[0];   // empty == nothing yet
            byte[] head = b.m_rx.peekFirst();
            if (head.length <= maxBytes) { b.m_rx.removeFirst(); b.m_rxBytes -= head.length; return head; }
            byte[] part = new byte[maxBytes];
            System.arraycopy(head, 0, part, 0, maxBytes);
            byte[] rest = new byte[head.length - maxBytes];
            System.arraycopy(head, maxBytes, rest, 0, rest.length);
            b.m_rx.removeFirst(); b.m_rx.addFirst(rest); b.m_rxBytes -= maxBytes;
            return part;
        }
    }

    public static void flush()
    {
        UsbSerialBridge b = inst();
        synchronized (b.m_rxLock) { b.m_rx.clear(); b.m_rxBytes = 0; }
    }

    public static int available()
    {
        UsbSerialBridge b = inst();
        synchronized (b.m_rxLock) { return b.m_rxBytes; }
    }

    public static boolean isOpen()      { return inst().m_open; }
    public static String  lastError()   { return inst().m_lastError; }

    // Support diagnostics. Deliberately NO serial number: it identifies the
    // unit without helping a support case.
    public static boolean hasUsbHostFeature()
    {
        try {
            return s_activity != null && s_activity.getPackageManager()
                .hasSystemFeature("android.hardware.usb.host");
        } catch (Throwable t) { return false; }
    }

    public static String deviceProductName(int vid, int pid)
    {
        try {
            UsbDevice d = inst().findDevice(vid, pid);
            if (d == null) return "";
            String n = d.getProductName();
            return n == null ? "" : n;
        } catch (Throwable t) { return ""; }
    }
}
