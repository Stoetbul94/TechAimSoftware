#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Session Reliability Layer — M0: platform durability boundary.
//
// The ONE place that knows how to push file bytes past the OS page cache.
// Callers hold an OPEN QFile; syncFile() forces its contents to stable
// storage. QtCore only.
//
// Current behaviour (M0):
//   Windows : FlushFileBuffers() on the file's Win32 handle.
//   POSIX / Android : fsync() on the file descriptor (compiles when the
//                     Android/Linux port is built; same call site).
//   Other/unsupported platforms : returns the result of QFile::flush() only —
//     data reaches the OS but hard-power-loss durability is NOT guaranteed;
//     callers treat that as a degraded probe result.
//
// M0 establishes the boundary only. The full per-event durability policy
// (which events fsync, watchdog, threading graduation) is M1/M2 —
// docs/session-reliability-implementation-spec.md §11/§12.
// ─────────────────────────────────────────────────────────────────────────────

#include <QFile>

namespace ta {
namespace rel {

class StorageSync {
public:
    // Flush Qt buffers and force OS buffers for `file` (must be open).
    // Returns false if flushing or the platform sync call failed.
    static bool syncFile(QFile& file);
};

} // namespace rel
} // namespace ta
