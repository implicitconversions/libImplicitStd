// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

// This header is a place for our Windows wstring things.

#include <cstdio>
#include <wchar.h>
#include <cstdarg>

// snwprintf is usually preferred over other variants:
//   - snwprintf_s has annoying parameter validation and nullifies the buffer instead of truncate.
//   - swprintf_s  has annoying parameter validation and nullifies the buffer instead of truncate.
//   - snwprintf   truncates the result and ensures a null terminator.
template<intmax_t _Size> __verify_fmt(2, 3)
int snwprintf(wchar_t (&_Buf)[_Size], wchar_t const* _Fmt, ...) {
	int _Res;
	va_list _Ap;

	va_start(_Ap, _Fmt);
	_Res = _vsnwprintf(_Buf, _Size-1, _Fmt, _Ap);
	_Buf[_Size-1] = 0;		// the _score version of microsoft's snprintf don't ensure null termination.
	va_end(_Ap);

	return _Res;
}
