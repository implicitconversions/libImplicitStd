#pragma once

#include "StdStringArg.h"

#include <cstdarg>

namespace StringUtil {

template<class StdStrT>
void AppendFmtV(StdStrT& result, const StringConversionMagick& fmt, va_list list)
{
	if (fmt.empty()) return;

	int destSize = 0;
	va_list argcopy;
	va_copy(argcopy, list);
	destSize = vsnprintf(nullptr, 0, fmt.c_str(), argcopy);
	va_end(argcopy);

	assertD(destSize >= 0, "Invalid string formatting parameters");

	// vsnprintf doesn't count terminating '\0', and resize() doesn't expect it either.
	// Thus, the following resize() will ensure +1 room for the null that vsprintf_s
	// will write.

	auto curlen = result.length();
	result.resize(destSize+curlen);
    #if defined(_MSC_VER)
	vsprintf_s
    #else
    vsnprintf
    #endif
    (const_cast<char*>(result.data() + curlen), destSize+1, fmt.c_str(), list );
}

template<typename T> __always_inline
void AppendFmt(T& result, const char* fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	StringUtil::AppendFmtV(result, fmt, list);
	va_end(list);
}

}
