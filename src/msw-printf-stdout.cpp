
#if PLATFORM_MSW
#define NOMINMAX
#include <Windows.h>

#include <ios>
#include <io.h>
#include <fcntl.h>

#include <algorithm>

#include <string_view>

// Convert an UTF8 string to a wide Unicode String
__nodebug static std::wstring utf8_decode(std::string_view str)
{
	if (str.empty()) return {};
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo( size_needed, 0 );
	MultiByteToWideChar                  (CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

__nodebug bool msw_IsDebuggerPresent()
{
	return ::IsDebuggerPresent() == TRUE;
}

__nodebug void msw_OutputDebugString(const char* fmt)
{
	// must use OutputDebugStringW() to see UTF8 sequences in the Output window.
	::OutputDebugStringW(utf8_decode(fmt).c_str());
}

void msw_OutputDebugStringV(const char* fmt, va_list args)
{
	// vsnprintf has what is arguably unexpected behavior: it returns the length of the formatted string,
	// not the # of chars written.  It also doesn't null-terminate strings if it runs out of buffer space.

	char buf[2048];
	int sizeof_buf = sizeof(buf);
	int len = vsnprintf(buf, sizeof_buf, fmt, args);
	buf[sizeof_buf-1] = '\0';		// in case vsnprintf overflowed.

	::OutputDebugStringW(utf8_decode(buf).c_str());

	if (len >= sizeof_buf) {
		// Things piped to stderr shouldn't be excessively verbose anyway
		::OutputDebugStringA("\n");
		::OutputDebugStringA("(*OutputDebugString*) previous message was truncated, see stderr console for full content");
	}
}
#if REDEFINE_PRINTF
#	undef printf
#	undef vfprintf
#	undef fprintf
#	undef puts
#	undef fputs
#	undef fputc
#   undef fwrite
#endif

static FILE* s_pipe_stdin;
static FILE* s_pipe_stdout;
static FILE* s_pipe_stderr;

void _fi_redirect_winconsole_handle(FILE* stdhandle, void* winhandle)
{
	FILE** pipedest = nullptr;
	char const* mode = nullptr;
	char const* reopen_fn = nullptr;

	// open outputs as binary to suppress Windows newline corruption (\r mess)

	if (stdhandle == stdin)  { pipedest = &s_pipe_stdin;  mode = "r";  reopen_fn = "CONIN$" ; }
	if (stdhandle == stdout) { pipedest = &s_pipe_stdout; mode = "wb"; reopen_fn = "CONOUT$"; }
	if (stdhandle == stderr) { pipedest = &s_pipe_stderr; mode = "wb"; reopen_fn = "CONOUT$"; }

	if (!pipedest || !mode) {
		// too early to throw asserts.
		return;
	}

	if (winhandle)
	{
		if (*pipedest) fclose(*pipedest);

		// record the existing pipe handle so that we can send interesting stuff there too.
		// To do so properly we use DuplicateHandle on the provided winhandle, otherwise the
		// process/app will close it our from under us StdStdHandle is called later.

		HANDLE dupHandle;
		auto winresult = DuplicateHandle(GetCurrentProcess(), winhandle,
			GetCurrentProcess(), &dupHandle , 0,
			FALSE, DUPLICATE_SAME_ACCESS
		);

		if (winresult) {
			int fd = _open_osfhandle((intptr_t)dupHandle, _O_BINARY);
			*pipedest = _fdopen(fd, mode);
			if (stdhandle == stdout) {
				setvbuf(*pipedest, NULL, _IOFBF, 32768);
			}
			else {
				setvbuf(*pipedest, NULL, _IONBF, 0);
			}
		}
		else {
			// log out the error, it'll hopefully show up in the piped file as a clue.
			if (stdhandle != stdin) {
				fprintf(stdhandle, "DuplicateHandle(%s) failed: Windows Error 0x%08x\nThis pipe will be closed/inactive as a result.", (stdhandle==stdout) ? "stdout" : "stderr", GetLastError());
			}
			else {
				fprintf(stderr, "DuplicateHandle(stdin) failed: Windows Error 0x%08x\n", GetLastError());
			}
		}
	}

	if (reopen_fn) {
		freopen(reopen_fn,  mode,  stdhandle);
	}
}


#if REDEFINE_PRINTF

extern "C" {
__nodebug int _fi_redirect_printf(const char* fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	auto result = _fi_redirect_vfprintf(stdout, fmt, argptr);
	va_end(argptr);
	return result;
}

__nodebug int _fi_redirect_fprintf(FILE* handle, const char* fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	auto result = _fi_redirect_vfprintf(handle, fmt, argptr);
	va_end(argptr);
	return result;
}

// in all cases where this is used, we honor result from original pipe, since it might have EOF restrictions
// (and it's very unlikely the windows console would).
static FILE* getOriginalPipeHandle(FILE* stdhandle)
{
	if ((s_pipe_stdout && stdhandle == stdout)) return s_pipe_stdout;
	if ((s_pipe_stderr && stdhandle == stderr)) return s_pipe_stderr;
	return nullptr;
}

int _fi_redirect_vfprintf(FILE* handle, const char* fmt, va_list args)
{
	int result;
	if (1) {
		// flush stdout before writing stderr, otherwise the context of stderr will be misleading.
		if (handle == stderr) {
			fflush(stdout);
			if (s_pipe_stdout) {
				fflush(s_pipe_stdout);
			}
		}

		va_list argptr;
		va_copy(argptr, args);
		result = vfprintf(handle, fmt, argptr);
		va_end(argptr);
	}

	if (1) {
		if (FILE* pipeto = getOriginalPipeHandle(handle)) {
			va_list argptr;
			va_copy(argptr, args);
			result = vfprintf(pipeto, fmt, argptr);
			va_end(argptr);
		}
	}

	if (handle == stdout || handle == stderr)
	{
		if (msw_IsDebuggerPresent()) {
			va_list argptr;
			va_copy(argptr, args);
			msw_OutputDebugStringV(fmt, argptr);
			va_end(argptr);
		}
	}
	return result;
}

int _fi_redirect_puts(char const* buffer) {
	// since puts appends a newline and OutputDebugString doesn't have a formatted version,
	// it's easier to just fall back on our printf implementation.
	return _fi_redirect_printf("%s\n", buffer);
}

int _fi_redirect_fputs(char const* buffer, FILE* handle) {
	int result = fputs(buffer, handle);

	if (FILE* pipeto = getOriginalPipeHandle(handle)) {
		result = fputs(buffer, pipeto);
	}

	if (handle == stdout || handle == stderr)
	{
		if (msw_IsDebuggerPresent()) {
			msw_OutputDebugString(buffer);
		}
	}
	return result;
}

int _fi_redirect_fputc(int ch, FILE* handle) {
	int result = fputc(ch, handle);

	if (FILE* pipeto = getOriginalPipeHandle(handle)) {
		result = fputc(ch, pipeto);
	}

	if (handle == stdout || handle == stderr)
	{
		if (msw_IsDebuggerPresent()) {
			// if someone passes a UTF32 char in then we have to do something annoying here to
			// convert it to UTF8, I think. ugh. For now I'm happy to just handle the common case, '\n'
			char meh[2] = { (char)ch, 0 };
			msw_OutputDebugString(meh);
		}
	}
	return result;
}

intmax_t _fi_redirect_fwrite(void const* buffer, size_t size, size_t nelem, FILE* handle)
{
	auto result = fwrite(buffer, size, nelem, handle);

	if (FILE* pipeto = getOriginalPipeHandle(handle)) {
		result = fwrite(buffer, size, nelem, pipeto);
	}

	if (handle == stdout || handle == stderr)
	{
		if (msw_IsDebuggerPresent()) {
			char mess[2048];
			auto nsize = size * nelem;
			auto* chbuf = (const char*)buffer;
			while(nsize) {
				auto chunksize = std::min(nsize, 1023ULL);
				memcpy(mess, chbuf, chunksize);
				mess[chunksize] = 0;
				msw_OutputDebugString(mess);
				nsize -= chunksize;
				chbuf += chunksize;
			}
		}
	}
	return result;
}
}
#endif
#endif
