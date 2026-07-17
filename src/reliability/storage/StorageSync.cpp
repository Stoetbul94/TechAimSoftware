#include "StorageSync.h"

#if defined(Q_OS_WIN)
#  include <qt_windows.h>
#  include <io.h>
#elif defined(Q_OS_UNIX)
#  include <unistd.h>
#endif

namespace ta {
namespace rel {

bool StorageSync::syncFile(QFile& file)
{
    if (!file.isOpen())
        return false;
    if (!file.flush())
        return false;

    const int fd = file.handle();
    if (fd < 0)
        return false;

#if defined(Q_OS_WIN)
    const HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    if (h == INVALID_HANDLE_VALUE)
        return false;
    return ::FlushFileBuffers(h) != 0;
#elif defined(Q_OS_UNIX)
    return ::fsync(fd) == 0;
#else
    // Unsupported platform: Qt-level flush already happened above. Durability
    // past the OS cache is not guaranteed here — documented M0 fallback.
    return true;
#endif
}

} // namespace rel
} // namespace ta
