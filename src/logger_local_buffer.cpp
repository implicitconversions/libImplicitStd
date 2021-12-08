#include "logger_local_buffer.h"
#include <cstdarg>

void logger_local_buffer::append(const char* msg) {
	if (!msg) return;

	// strncpy and strcpy_s have either perf or logical problems, so we have to
	// roll our own that does what we want...
	auto trystrcpy = [](char* dest, int destlen, const char* src) {
		if (!destlen) return 0;

		char* ret = dest;
		int pos = 0;
		while(pos < destlen)
		{
			dest[pos] = src[pos];
			if (!src[pos]) return pos;
			++pos;
		}
		// truncation scenario
		return destlen-1;
	};

	if (!longbuf) {
		int written = trystrcpy(buffer+wpos, bufsize-wpos, msg);
		wpos += written;
		if (written >= bufsize-wpos-1) {
			// heapify things, using prefix from above and full message
			// wpos value becomes unimportant once longbuf is setup.
			longbuf = new std::string(buffer,wpos);
		}
	}

	if (longbuf) {
		*longbuf += msg;
	}
}

void logger_local_buffer::appendfv(const char* fmt, va_list args) {
	if (!fmt) return;

	int expected_len = 0;
	if (!longbuf) {
		va_list argptr;
		va_copy(argptr, args);
		expected_len = vsnprintf(buffer+wpos, bufsize - wpos, fmt, argptr);
		va_end(argptr);

		if (expected_len >= bufsize - wpos - 1) {
			longbuf = new std::string(buffer,wpos);
		}
		else {
			wpos += expected_len;
			buffer[wpos] = 0;
		}
	}

	if (longbuf) {
		va_list argptr;
		va_copy(argptr, args);
		if (!expected_len) {
			expected_len = vsnprintf(nullptr, 0, fmt, argptr);
		}
		auto longsz = longbuf->size();
		longbuf->resize(longsz+expected_len);
		vsprintf_s(const_cast<char*>(longbuf->data() + longsz), expected_len+1, fmt, argptr);
		va_end(argptr);
	}
}

void logger_local_buffer::append(char ch) {
	if (!longbuf) {
		if (wpos >= bufsize - 1) {
			longbuf = new std::string(buffer,wpos);
		}
		else {
			buffer[wpos+0] = ch;
			buffer[wpos+1] = 0;
			++wpos;
		}
	}

	if (longbuf) {
		longbuf->append(1, ch);
	}
}

void logger_local_buffer::write_to(FILE* pipe) const
{
	// windows will flush stdout and stderr out-of-order if we don't explicitly flush everything first.
	if (pipe == stderr) { fflush(nullptr); }
	fputs(longbuf ? longbuf->c_str() : buffer, pipe);
	if (pipe == stderr) { fflush(nullptr); }
}

void logger_local_buffer::clear() {
	wpos = 0;
	buffer[0] = 0;
	if (longbuf) {
		longbuf->clear();
	}
}

void logger_local_buffer::formatv(const char* fmt, va_list args) {
	clear();
	appendfv(fmt, args);
}

void logger_local_buffer::appendf(const char* fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	appendfv(fmt, argptr);
	va_end(argptr);
}

void logger_local_buffer::format(const char* fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	formatv(fmt, argptr);
	va_end(argptr);
}
