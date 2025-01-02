// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

#include "icy_assert.h"
#include "icyAppErrorCallback.h"
#include "StringBuilder.h"

#include <cstdarg>

// We intentionally use not-the-last-named-parameter here, to avoid needing to string-builder.
MSC_WARNING_DISABLE(5082);		// second argument to 'va_start' is not the last named parameter

// Default assertion and error reporting mechanism. Simple logging to console (stderr).
static bool _libimplicit_app_report_error(char const* filepos, char const* condmsg, char const* fmt, ...) {

	logger_local_buffer buf;
	buf.appendf("%s:%s: ", filepos, condmsg);
	if (fmt) {
		va_list ap;
		va_start(ap, fmt);
		buf.appendfv(fmt, ap);
		va_end(ap);
	}
	fflush(stdout);
	fprintf(stderr, "%s\n", buf.c_str());

	// return TRUE to abort, FALSE to ignore.
	// our default behavior only supports abort currently.
	// (ignore would be tied to a hash of the filepos string).
	return 1;
}

// This can be reassigned prior ot any static initializers by __section_item_ro (LinkSectionComdat.h)
// Note that the module invoking __section_item_ro must be directly linked to the executable (not via .lib)
FnType_IcyAppErrorCallback* libimplicit_app_report_error = _libimplicit_app_report_error;
