// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

#if !defined(REDEFINE_PRINTF)
#	if defined(_WIN32)
#		define REDEFINE_PRINTF		1
#	endif
#endif

#if !defined(__verify_fmt)
#	if defined(_MSC_VER)
#		define __verify_fmt(fmtpos, vapos)
#	else
#		define __verify_fmt(fmtpos, vapos)  __attribute__ ((format (printf, fmtpos, vapos)))
#	endif
#endif

// On Windows it's handy to have printf() automatically redirect itself into the Visual Studio Output window
// and the only sane way to accomplish that seems to be by replacing printf() with a macro.
#if REDEFINE_PRINTF
#include <stdio.h>		// must include before macro definition
#if defined(__cplusplus)
#  include <cstdint>
#	define _extern_c		extern "C"
#else
#  include <stdint.h>
#	define _extern_c
#endif

// these functions must be defined inside the std:: namespace to allow for code that accesses things using
// "std::fwrite" to work in an expected manner.
#if defined(__cplusplus)
namespace std {
#endif
_extern_c int      _fi_redirect_printf   (const char* fmt, ...);
_extern_c int      _fi_redirect_vprintf  (const char* fmt, va_list args);
_extern_c int      _fi_redirect_vfprintf (FILE* handle, const char* fmt, va_list args);
_extern_c int      _fi_redirect_fprintf  (FILE* handle, const char* fmt, ...);
_extern_c int      _fi_redirect_puts     (char const* _Buffer);
_extern_c int      _fi_redirect_fputs    (char const* _Buffer, FILE* _Stream);
_extern_c int      _fi_redirect_fputc    (int ch, FILE* _Stream);
_extern_c intmax_t _fi_redirect_fwrite   (void const* src, size_t, size_t, FILE* fp);
_extern_c void _fi_redirect_winconsole_handle(FILE* stdhandle, void* winhandle);	// expects result of GetStdHandle
#if defined(__cplusplus)
}
using std::_fi_redirect_printf   ;
using std::_fi_redirect_vprintf  ;
using std::_fi_redirect_vfprintf ;
using std::_fi_redirect_fprintf  ;
using std::_fi_redirect_puts     ;
using std::_fi_redirect_fputs    ;
using std::_fi_redirect_fputc    ;
using std::_fi_redirect_fwrite   ;
using std::_fi_redirect_winconsole_handle;
#endif


// implementation notes:
//  - microsoft doesn't enable the new preprocessor for C even if you request it. (VS2019 checked)
//    MSC also has some issues handling our VA_OPT(,) workaround, failin to paste the comma correctly (apparently
//    deleting it for us "as a favor"?), which prevents using the clever macro-based workaround.
//  - the redirected implementations typically ensure stdout is flushed before writing to stderr, so our error
//    macros here can skip that paperwork.

#if defined(errprintf)
#	undef errprintf
#endif

#if __cplusplus && !_MSVC_TRADITIONAL
#	define printf(fmt, ...)			_fi_redirect_printf  (           fmt __VA_OPT__(,) ## __VA_ARGS__)
#	define fprintf(fp, fmt, ...)	_fi_redirect_fprintf (fp,        fmt __VA_OPT__(,) ## __VA_ARGS__)
#	define errprintf(fmt, ...)		_fi_redirect_fprintf (stderr, "" fmt __VA_OPT__(,) ## __VA_ARGS__)
#else
#	define printf(fmt, ...)			_fi_redirect_printf  (           fmt, ## __VA_ARGS__)
#	define fprintf(fp, fmt, ...)	_fi_redirect_fprintf (fp,        fmt, ## __VA_ARGS__)
#	define errprintf(fmt, ...)		_fi_redirect_fprintf (stderr, "" fmt, ## __VA_ARGS__)
#endif

#define vprintf(fmt, args)	    _fi_redirect_vprintf (fmt, args)
#define vfprintf(fp, fmt, args)	_fi_redirect_vfprintf(fp, fmt, args)
#define puts(msg)				_fi_redirect_puts    (msg)
#define fputs(msg, fp)			_fi_redirect_fputs   (msg, fp)
#define fputc(ch, fp)			_fi_redirect_fputc   (ch, fp)
#define fwrite(buf, sz, nx, fp) _fi_redirect_fwrite  (buf, sz, nx, fp)

#undef _extern_c
#endif		// REDEFINE_PRINTF
