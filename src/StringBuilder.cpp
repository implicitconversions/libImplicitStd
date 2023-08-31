#include "StringBuilder.h"
#include "StringBuilder.hxx"

// strncpy and strcpy_s have either perf or logical problems, so we have to
// roll our own that does what we want:
//   1. return full length of string on trunc (we use this to heap alloc it)
//   2. don't bother null-terminating on trunc (because it's going to heap)

template<bool AllowHeapFallback>
int _internalBufferFormatterImpl<AllowHeapFallback>::trystrcpy(char* dest, int destlen, const char* src) {
	if (!destlen) return 0;

	char* ret = dest;
	int pos = 0;
	while(pos < destlen)
	{
		dest[pos] = src[pos];
		if (!src[pos]) return pos;
		++pos;
	}

	// truncation scenario, count out full length of string.
	while(src[pos++]);

	return pos;
}

// pipe should be either stdout or stderr. When stderr is recieved, flushing is performed to
// ensure the output of stdout/stderr to console is coherent.
void WriteToStandardPipeWithFlush(FILE* pipe, char const* msg)
{
	// windows will flush stdout and stderr out-of-order if we don't explicitly flush stdout first.
	// also, stderr should be unbuffered, but this behavior can vary by platform.

	if (pipe == stderr) { fflush(stdout); }
	fputs(msg, pipe);
	if (pipe == stderr) { fflush(stderr); }
}


template class _internalBufferFormatterImpl<true>;
template class _internalBufferFormatterImpl<false>;

template class StringBuilder<256>;
template class StringBuilder<512>;
template class StringBuilder<2048>;
