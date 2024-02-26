#pragma once

// Placeholder for the libImplicitStd library assertion interface.
// These can interface to internal assert() macros or invoke abort() directly.
// It is recommended to discard it and provide macro redirection to your own preferred assertion
// framework. Redirection looks like so:

/*
#include "my-assertions.h"

#define assertD(cond, ...)     (my_assert( cond, ## __VA_ARGS ))
#define assertH(cond, ...)     (my_assert( cond, ## __VA_ARGS ))

#define app_abort(...)         (my_abort ( ## __VA_ARGS__ ))
*/

#include <cstdlib>      // for abort()
#include <cstdio>

#include "MacroUtil.h"
#include "icyAppErrorCallback.h"


#if !defined(ENABLE_DEBUG_CHECKS)
#	define ENABLE_DEBUG_CHECKS		(1)
#endif

// ENABLE_HOUSE_CHECKS: turns on inhouse assertion checks (semi-popular game studio nomenclature, refers
// to builds for internal development and QA, usually less aggressively checked than debug but not entirely
// without any assertions at all.
#if !defined(ENABLE_HOUSE_CHECKS)
#	define ENABLE_HOUSE_CHECKS		(ENABLE_DEBUG_CHECKS)
#endif


// Implementation Note: DO NOT EXPAND 'cond' INTO assertFmtR. This causes full expansion of the condition,
// and if it's a macro it'll end up being potentially several lines of C code after macro expansion.

#if !defined(assertFmtR)
#	define assertFmtR(cond, filepos, condStr, ...)		((cond) || (libimplicit_app_report_error(filepos, condStr, ## __VA_ARGS__, nullptr ) && (abort(),0)))
#endif


#if !defined(app_abort)
#	define app_abort(...)				assertFmtR(false, __ICY_FILEPOS__, "ABORT", ## __VA_ARGS__)
#endif


#if !defined(assertR)
#	define assertR(cond, ...)			assertFmtR(cond, __ICY_FILEPOS__, "ASSERT (" # cond ")", ## __VA_ARGS__)
#endif

#if !defined(assertD)
#	if ENABLE_DEBUG_CHECKS
#		define assertD(cond, ...)		assertFmtR(cond, __ICY_FILEPOS__, "ASSERT (" # cond ")", ## __VA_ARGS__)
#	else
#		define assertD(cond, ...)		((void)0)
#	endif
#endif

#if !defined(assertH)
#	if ENABLE_HOUSE_CHECKS
#		define assertH(cond, ...)		assertFmtR(cond, __ICY_FILEPOS__, "ASSERT (" # cond ")", ## __VA_ARGS__)
#	else
#		define assertH(cond, ...)		((void)0)
#	endif
#endif

#if ENABLE_HOUSE_CHECKS
#	define icyReportError(...)			assertFmtR(false, __ICY_FILEPOS__, "ReportError", __VA_ARGS__ ), 0
#else
#	define icyReportError(...)			(libimplicit_app_report_error(__ICY_FILEPOS__, "ReportError", __VA_ARGS__ ))
#endif
