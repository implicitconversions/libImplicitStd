#pragma once

// Placeholder for the icy-gist library's assertion interface.
// These can interface to internal assert() macros, or invoke abort() directly, as shown.
// It is recommended to discard it and provide macro redirection to your own preferred assertion
// framework. Redirection looks like so:

/*
#include "my-assertions.h"

#define dbg_check(cond, ...)     (my_assert( ## __VA_ARGS ))
#define house_check(cond, ...)     (my_assert( ## __VA_ARGS ))

#define dbg_abort(...)           (my_abort ( ## __VA_ARGS ))
#define house_abort(...)           (my_abort ( ## __VA_ARGS ))
*/

#include <cstdlib>      // for abort()
#include <cstdio>

#define ICY_TRACE2(f,l) f "(" # l "): "
#define ICY_TRACE1(f,l) ICY_TRACE2(f,l)
#define ICY_TRACE       ICY_TRACE1(__FILE__, __LINE__)

#if !defined(app_abort)
#	define app_abort(...)		    (fprintf(stderr, ICY_TRACE "(abort) " __VA_ARGS__ ), abort(),0)
#endif

#if !defined(assertR)
#	define assertR(cond, ...)		((cond) || (fprintf(stderr, ICY_TRACE "(assert failed) " # cond ": " __VA_ARGS__ ), abort(),0))
#endif

#if !defined(assertD)
#	if ENABLE_DEBUG_ASSERT
#		define assertD(cond, ...)		assertR(cond, __VA_ARGS__)
#	else
#		define assertD(cond, ...)
#	endif
#endif

#if !defined(assertH)
#	if ENABLE_HOUSE_ASSERT
#		define assertH(cond, ...)		assertR(cond, __VA_ARGS__)
#	else
#		define assertH(cond, ...)
#	endif
#endif
