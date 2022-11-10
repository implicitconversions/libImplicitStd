#pragma once

#include <string>

// TODO: someday make std::string here templatable.
template<bool AllowHeapFallback>
class _internalBufferFormatterImpl {
private:
	_internalBufferFormatterImpl(const _internalBufferFormatterImpl&) = delete;
	_internalBufferFormatterImpl(_internalBufferFormatterImpl&&)      = delete;
	_internalBufferFormatterImpl& operator=(const _internalBufferFormatterImpl&) = delete;
	_internalBufferFormatterImpl& operator=(_internalBufferFormatterImpl&&)      = delete;

public:
	char* buffer;
	int   bufsize;
	int&  wpos;
	std::string** longbuf;

	_internalBufferFormatterImpl(
		char* _buffer,
		int   _bufsize,
		int&  _wpos,
		std::string** _longbuf = nullptr
	);

	void clear    ();
	void append   (const char* msg);
	void appendfv (const char* fmt, va_list args);
	void formatv  (const char* fmt, va_list args);
//	void appendf  (const char* fmt, ...);
//	void format   (const char* fmt, ...);
	void append   (char c);

protected:
	void heapify_longbuf();
	void heapify_longbuf_append(int written);
	void longbuf_clear();
	void longbuf_append(char const* msg);
	void longbuf_append(char ch);
	void longbuf_appendfv(int expected_len, char const* msg, va_list args);
	int trystrcpy(char* dest, int destlen, const char* src);
};

using _internalFormatterSpillToHeap = _internalBufferFormatterImpl<true>;
using _internalFormatterTruncate    = _internalBufferFormatterImpl<false>;

template<int bufsize=256>
class StringBuilderTrunc {
private:
	StringBuilderTrunc(const StringBuilderTrunc&) = delete;
	StringBuilderTrunc(StringBuilderTrunc&&)      = delete;
	StringBuilderTrunc& operator=(const StringBuilderTrunc&) = delete;
	StringBuilderTrunc& operator=(StringBuilderTrunc&&)      = delete;

public:
	char			buffer[bufsize];
	int				wpos	= 0;

	StringBuilderTrunc() {
		buffer[0] = 0;
	}

	void clear    ();
	StringBuilderTrunc& append	(const char* msg);
	StringBuilderTrunc& appendfv (const char* fmt, va_list args);
	StringBuilderTrunc& formatv  (const char* fmt, va_list args);
	StringBuilderTrunc& appendf  (const char* fmt, ...)          __verify_fmt(2,3);
	StringBuilderTrunc& format   (const char* fmt, ...)          __verify_fmt(2,3);
	StringBuilderTrunc& append   (char c);

	char const* c_str() const {
		return buffer;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// StringBuilder
//
// Uses a local buffer for formatting short strings. Falls back to HEAP for large strings.
// 
// This is considered a mission-critical string formatting facility. Avoids heap alloc for the
// common-case strings under a specified length. This class is intended to remain simple and free
// of feature creep. Other mission critical components such as the logging facility depend on it.
//
// (one interesting idea here might be to provide a template for the heap-allocated std::string to
//  allow routing it to an alternative heap)
//
template<int bufsize=256>
class StringBuilder
{
private:
	StringBuilder(const StringBuilder&) = delete;
	StringBuilder(StringBuilder&&)      = delete;
	StringBuilder& operator=(const StringBuilder&) = delete;
	StringBuilder& operator=(StringBuilder&&)      = delete;

public:
	char			buffer[bufsize];
	int				wpos	= 0;
	std::string*	longbuf = nullptr;					// used only if buffer[] exceeded.

	StringBuilder() {
		buffer[0] = 0;
	}

	void clear    ();
	StringBuilder& append	(const char* msg);
	StringBuilder& appendfv (const char* fmt, va_list args);
	StringBuilder& formatv  (const char* fmt, va_list args);
	StringBuilder& appendf  (const char* fmt, ...)          __verify_fmt(2,3);
	StringBuilder& format   (const char* fmt, ...)          __verify_fmt(2,3);
	StringBuilder& append   (char c);

	char const* c_str() const {
		return longbuf ? longbuf->c_str() : buffer;
	}

	~StringBuilder() {
		delete longbuf;
	}
};

#if defined(VERIFY_PRINTF_ON_MSVC)
#	define cFmtStrLocal(...) (VERIFY_PRINTF_ON_MSVC(__VA_ARGS__),  StringBuilder<512>().format(__VA_ARGS__).c_str())
#else
#	define cFmtStrLocal(...) ( StringBuilder<512>().format(__VA_ARGS__).c_str())
#endif

// local stack-storage string buffer that spills to heap for long strings.
template<int bufsize>
using StringBuf = StringBuilder<bufsize>;

// non-growable local stack storage string buffer that truncates (never uses heap).
template<int bufsize>
using StringBufTrunc = StringBuilderTrunc<bufsize>;

// Mission critical logging facility. Avoids heap alloc for the common-case string printout.
// Falls back on heap alloc for large buffers. Effectively a lightweight temporary-scope string
// builder with a few logger-specific functions.
using logger_local_buffer = StringBuilder<2048>;

void WriteToStandardPipeWithFlush(FILE* pipe, char const* msg);		// maybe find a better home for this.

#if !USE_SCE_LTO
#	include "StringBuilder.hxx"
#endif
