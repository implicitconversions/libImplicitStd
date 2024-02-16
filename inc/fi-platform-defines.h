// Contents released under the The MIT License (MIT)

#pragma once

// Default selection of MSW platform is based on using the Microsoft Compiler.
// This is not a foolproof assumption, but a special environment can specify these
// defines explicitly via makefile.

#if !defined(PLATFORM_MSW)
#   if defined(_WIN32)
#       define PLATFORM_MSW     1
#   else
#       define PLATFORM_MSW     0
#   endif
#endif

#if !defined(PLATFORM_POSIX)
#	if PLATFORM_MSW
#		define PLATFORM_POSIX 0
#	elif __has_include(<unistd.h>)
#		define PLATFORM_POSIX 1
#	else
#		define PLATFORM_POSIX 0
#	endif
#endif

#if !defined(PLATFORM_LINUX)
#   ifdef __linux__
#		define PLATFORM_LINUX 1
#	else
#		define PLATFORM_LINUX 0
#	endif
#endif

#if !defined(PLATFORM_MSCRT)
#   if defined(_WIN32)
#       define PLATFORM_MSCRT   1
#   else
#       define PLATFORM_MSCRT   0
#   endif
#endif

#define PLATFORM_HAS_GETENV (PLATFORM_MSW || PLATFORM_LINUX || PLATFORM_MAC)

#if __has_include("fi-platform-defines-sce.h")
#	include "fi-platform-defines-sce.h"
#endif

