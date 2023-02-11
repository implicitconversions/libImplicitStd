#pragma once

#include "StringBuilder.h"

#include <cstdarg>
#include <cassert>

template<bool AllowHeapFallback>
_internalBufferFormatterImpl<AllowHeapFallback>::_internalBufferFormatterImpl(
	char* _buffer,
	int   _bufsize,
	int&  _wpos,
	std::string** _longbuf
): wpos(_wpos) {
	if constexpr (AllowHeapFallback) {
		assert(_longbuf);
	}

	buffer	= _buffer;
	bufsize	= _bufsize;
	longbuf = _longbuf;
}

template<bool AllowHeapFallback> __noinline
void _internalBufferFormatterImpl<AllowHeapFallback>::longbuf_clear() {
	if constexpr(AllowHeapFallback) {
		longbuf[0]->clear();
	}
}

template<bool AllowHeapFallback> __noinline
void _internalBufferFormatterImpl<AllowHeapFallback>::heapify_longbuf() {
	if constexpr(AllowHeapFallback) {
		longbuf[0] = new std::string();
	}
}

template<bool AllowHeapFallback> __noinline
void _internalBufferFormatterImpl<AllowHeapFallback>::heapify_longbuf_append(int written) {
	if constexpr(AllowHeapFallback) {
		// heapify it (slowpath)
		longbuf[0] = new std::string();
		longbuf[0]->reserve(wpos+7);		// +7 is good for heaps built on 16 or 32 byte alignments.
		longbuf[0]->assign(buffer, wpos - written);
	}
}

template<bool AllowHeapFallback> __noinline
void _internalBufferFormatterImpl<AllowHeapFallback>::longbuf_append(char const* msg) {
	*longbuf[0] += msg;
}

template<bool AllowHeapFallback> __noinline
void _internalBufferFormatterImpl<AllowHeapFallback>::longbuf_append(char ch) {
	longbuf[0]->append(1, ch);
}

template<bool AllowHeapFallback> __noinline
void _internalBufferFormatterImpl<AllowHeapFallback>::longbuf_appendfv(int expected_len, char const* fmt, va_list args) {
	if constexpr(AllowHeapFallback) {
		va_list argptr;
		va_copy(argptr, args);
		if (!expected_len) {
			expected_len = vsnprintf(nullptr, 0, fmt, argptr);
		}
		auto longsz = longbuf[0]->size();
		longbuf[0]->resize(longsz+expected_len);
		vsprintf_s(const_cast<char*>(longbuf[0]->data() + longsz), expected_len+1, fmt, argptr);
		va_end(argptr);
	}
}

template<bool AllowHeapFallback> __always_inline
void _internalBufferFormatterImpl<AllowHeapFallback>::append(const char* msg) {
	if (expect_false(!msg || !msg[0])) return;

	if (!AllowHeapFallback || expect_true(!longbuf[0])) {
		int written = trystrcpy(buffer+wpos, bufsize-wpos, msg);
		wpos += written;
		if (expect_true(wpos <= bufsize-1)) {
			return;
		}
		else if constexpr (AllowHeapFallback) {
			heapify_longbuf_append(written);
		}
		else {
			wpos = bufsize-1;
		}
	}

	if constexpr(AllowHeapFallback) {
		if (expect_false(longbuf[0])) {
			longbuf_append(msg);
		}
	}
}

template<bool AllowHeapFallback>
void _internalBufferFormatterImpl<AllowHeapFallback>::append(char ch) {
	if (!AllowHeapFallback || expect_true(!longbuf[0])) {
		if (wpos < bufsize - 1) {
			buffer[wpos+0] = ch;
			buffer[wpos+1] = 0;
			++wpos;
		}
		else if constexpr (AllowHeapFallback) {
			heapify_longbuf_append(0);
		}
	}

	if constexpr(AllowHeapFallback) {
		if (expect_false(longbuf[0])) {
			longbuf_append(ch);
		}
	}
}


template<bool AllowHeapFallback> __always_inline
void _internalBufferFormatterImpl<AllowHeapFallback>::appendfv(const char* fmt, va_list args) {
	if (expect_false(!fmt || !fmt[0])) return;

	int expected_len = 0;
	if (!AllowHeapFallback || expect_true(!longbuf[0])) {
		va_list argptr;
		va_copy(argptr, args);
		expected_len = vsnprintf(buffer+wpos, bufsize - wpos, fmt, argptr);
		va_end(argptr);

		wpos += expected_len;
		if (expect_true(wpos <= bufsize-1)) {
			return;
		}
		else if constexpr(AllowHeapFallback) {
			// regular heapification - reserve/alloc happens below.
			heapify_longbuf();
		}
		else {
			wpos = bufsize-1;
		}
	}

	if constexpr(AllowHeapFallback) {
		if (expect_false(longbuf[0])) {
			longbuf_appendfv(expected_len, fmt, args);
		}
	}
}

template<bool AllowHeapFallback> __always_inline
void _internalBufferFormatterImpl<AllowHeapFallback>::clear() {
	wpos = 0;
	buffer[0] = 0;
	if constexpr(AllowHeapFallback) {
		if (expect_false(longbuf[0])) {
			longbuf_clear();
		}
	}
}

template<bool AllowHeapFallback> __always_inline
void _internalBufferFormatterImpl<AllowHeapFallback>::formatv(const char* fmt, va_list args) {
	clear();
	return appendfv(fmt, args);
}


/////////////////////////////////////////////////////////////////////////////////////////////////

template<int bufsize> __always_inline
void StringBuilder<bufsize>::clear() {
	_internalFormatterSpillToHeap{buffer, bufsize, wpos, &longbuf}.clear();
}

template<int bufsize> __always_inline
StringBuilder<bufsize>& StringBuilder<bufsize>::append(const char* msg) {
	_internalFormatterSpillToHeap{buffer, bufsize, wpos, &longbuf}.append(msg);
	return *this;
}

template<int bufsize> __always_inline
StringBuilder<bufsize>& StringBuilder<bufsize>::append(char ch) {
	_internalFormatterSpillToHeap{buffer, bufsize, wpos, &longbuf}.append(ch);
	return *this;
}

template<int bufsize> __always_inline
StringBuilder<bufsize>& StringBuilder<bufsize>::appendfv(const char* fmt, va_list args) {
	_internalFormatterSpillToHeap{buffer, bufsize, wpos, &longbuf}.appendfv(fmt, args);
	return *this;
}

template<int bufsize> __always_inline
StringBuilder<bufsize>& StringBuilder<bufsize>::appendf(const char* fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	appendfv(fmt, argptr);
	va_end(argptr);
	return *this;
}

template<int bufsize> __always_inline
StringBuilder<bufsize>& StringBuilder<bufsize>::formatv(const char* fmt, va_list args) {
	clear();
	return appendfv(fmt, args);
}

template<int bufsize> __always_inline
StringBuilder<bufsize>& StringBuilder<bufsize>::format(const char* fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	formatv(fmt, argptr);
	va_end(argptr);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

template<int bufsize> __always_inline
void StringBuilderTrunc<bufsize>::clear() {
	_internalFormatterTruncate{buffer, bufsize, wpos}.clear();
}

template<int bufsize> __always_inline
StringBuilderTrunc<bufsize>& StringBuilderTrunc<bufsize>::append(const char* msg) {
	_internalFormatterTruncate{buffer, bufsize, wpos}.append(msg);
	return *this;
}

template<int bufsize> __always_inline
StringBuilderTrunc<bufsize>& StringBuilderTrunc<bufsize>::append(char ch) {
	_internalFormatterTruncate{buffer, bufsize, wpos}.append(ch);
	return *this;
}

template<int bufsize> __always_inline
StringBuilderTrunc<bufsize>& StringBuilderTrunc<bufsize>::appendfv(const char* fmt, va_list args) {
	_internalFormatterTruncate{buffer, bufsize, wpos}.appendfv(fmt, args);
	return *this;
}

template<int bufsize> __always_inline
StringBuilderTrunc<bufsize>& StringBuilderTrunc<bufsize>::appendf(const char* fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	appendfv(fmt, argptr);
	va_end(argptr);
	return *this;
}

template<int bufsize> __always_inline
StringBuilderTrunc<bufsize>& StringBuilderTrunc<bufsize>::formatv(const char* fmt, va_list args) {
	clear();
	return appendfv(fmt, args);
}

template<int bufsize> __always_inline
StringBuilderTrunc<bufsize>& StringBuilderTrunc<bufsize>::format(const char* fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	formatv(fmt, argptr);
	va_end(argptr);
	return *this;
}

