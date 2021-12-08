
#include "posix_file.h"
#include "icy_log.h"
#include "icy_assert.h"

#if PLATFORM_MSW

#define NOMINMAX
#define NO_STRICT
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <sys/stat.h>

#include <climits>
#include <algorithm>

size_t _pread(int fd, void* dest, size_t count, x_off_t pos)
{
    // windows POSIX libs are lacking the fancy new pread() function. >_<
    _lseeki64(fd, pos, SEEK_SET);
    auto orig_count = count;

    const auto int_max = 0x7fff'fffeULL;
    static_assert(int_max == (int)int_max, "Looks like int is not four bytes.");

    // because Windows _read is still stuck in the land of 322-bits.

    while (count > 0) {
        auto toread = std::min(count, int_max);
        auto amt = _read(fd, dest, (int)toread);
        count -= amt;
        dbg_check(count >= 0);
        if (amt != int_max) {
            return orig_count - count;
        }
    }
    return orig_count;
}

CStatInfo posix_fstat(int fd) {
    struct _stat64 sinfo;
    if (_fstat64 (fd, &sinfo) == -1) {
        dbg_check(false, "_fstat64(%d) failed, code=%d (%s)", fd, errno, strerror(errno));
        return {};
    }

    return {
        sinfo.st_mode,
        sinfo.st_size,

        sinfo.st_atime,
        sinfo.st_mtime,
        sinfo.st_ctime
    };
}

CStatInfo posix_stat(const char* path) {
    struct _stat64 sinfo;
    if (_stat64 (path, &sinfo) == -1) {
        //dbg_check(false, "_fstat64('%s') failed, code=%d (%s)", path, errno, strerror(errno));
        return {};
    }

    return {
        sinfo.st_mode,
        sinfo.st_size,

        sinfo.st_atime,
        sinfo.st_mtime,
        sinfo.st_ctime
    };
}

bool CStatInfo::IsFile     () const { return (st_mode & _S_IFREG) == _S_IFREG  ;}
bool CStatInfo::IsDir      () const { return (st_mode & _S_IFDIR) == _S_IFDIR  ;}
bool CStatInfo::Exists     () const { return (st_mode & _S_IFMT ) != 0         ;}


int posix_link(const char* existing_file, const char* link)
{
    _unlink(link);
    if (!CreateHardLinkA(link, existing_file, nullptr)) {
        //log_host("posix_link failed, win32code 0x%08x", GetLastError());
        return -1;
    }
    return 0;
}
#endif
