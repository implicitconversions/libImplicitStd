#pragma once

// Lightweight macro-based POSIX cross-compilation header

// off_t is too poorly defined to be of use. Let's define our own based on intmax_t.
using x_off_t = intmax_t;

#if PLATFORM_SCE    // Sony Playstation
#	include "posix_file_sce.h"

#elif PLATFORM_MSW
#	include <fcntl.h>
#	include <io.h>
#   include <cstdio>
#   include <sys/stat.h>

	// windows POSIX libs are lacking the fancy new pread() function. >_<
	extern size_t _pread(int fd, void* dest, size_t count, x_off_t pos);

	// Windows has this asinine non-standard notion of text mode POSIX files and, worse, makes the
	// non-standard behavior the DEFAULT behavior.  What the bloody hell, Microsoft?  Your're drunk.
	// Go Home.  --jstine

#	define posix_open(fn,flags,mode)   _open(fn,(flags) | _O_BINARY, mode)
#	define posix_read   _read
#	define posix_pread  _pread
#	define posix_write  _write
#	define posix_close  _close
#	define posix_lseek  _lseeki64
#	define posix_unlink _unlink
#	define O_DIRECT		(0)		// does not exist on windows
#	define DEFFILEMODE  (_S_IREAD | _S_IWRITE)

#elif PLATFORM_POSIX
#   include <fcntl.h>
#   include <unistd.h>
#   include <sys/stat.h>

#	define posix_open   open
#	define posix_read   read
#	define posix_pread  pread
#	define posix_write  write
#	define posix_close  close
#	define posix_lseek  lseek
#	define posix_unlink unlink

#   undef O_DIRECT
#   define O_DIRECT 0 // requires 512-byte alignment, must refactor target buffers first

#endif

#if !defined(posix_close)
#	error Unsupported platform.
#endif

struct CStatInfo;

// Modification info: abbreviated form of stat that excludes things not relevant to whether a file
// should be reloaded or not.
struct CStatModInfo {
	intmax_t    st_size;
	time_t      time_modified;
};

// Lightweight helper class for POSIX stat, just to put things in a little more friendly container.
struct CStatInfo
{
	uint32_t    st_mode;
	intmax_t    st_size;

	time_t      time_accessed;
	time_t      time_modified;
	time_t      time_changed;		// file contents -OR- mode changed.

	operator CStatModInfo() const {
		return { st_size, time_modified };
	}

	CStatModInfo getModificationInfo() const {
		return { st_size, time_modified };
	}

	bool IsFile     () const;
	bool IsDir      () const;
	bool Exists     () const;

	bool operator==(const CStatInfo& r) const {
		return (
			(st_mode		== r.st_mode			) &&
			(st_size		== r.st_size			) &&
			(time_accessed	== r.time_accessed		) &&
			(time_modified	== r.time_modified		) &&
			(time_changed	== r.time_changed		)
		);
	}

	bool operator!=(const CStatInfo& r) const {
		return !this->operator==(r);
	}
};


inline bool operator==(const CStatInfo& lval, const CStatModInfo& rval)  {
	return (
		(lval.st_size		== rval.st_size			) &&
		(lval.time_modified	== rval.time_modified	)
	);
}

inline bool operator==(const CStatModInfo& lval, const CStatInfo& rval)  {
	return (
		(lval.st_size		== rval.st_size			) &&
		(lval.time_modified	== rval.time_modified	)
	);
}

inline bool operator==(const CStatModInfo& lval, const CStatModInfo& rval)  {
	return (
		(lval.st_size		== rval.st_size			) &&
		(lval.time_modified	== rval.time_modified	)
	);
}

inline bool operator!=(const CStatModInfo& lval, const CStatInfo& rval) {
	return !operator==(lval, rval);
}


inline bool operator!=(const CStatInfo& lval, const CStatModInfo& rval) {
	return !operator==(lval, rval);
}


inline bool operator!=(const CStatModInfo& lval, const CStatModInfo& rval) {
	return !operator==(lval, rval);
}

extern int				posix_link		(const char* existing_file, const char* link);
extern CStatInfo		posix_fstat		(int fd);
extern CStatInfo		posix_stat		(const char* fullpath);
