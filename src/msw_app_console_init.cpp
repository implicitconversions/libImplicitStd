
#if PLATFORM_MSW
#include <Windows.h>
#include <Dbghelp.h>
#include <signal.h>
#include <cstring>
#include <cstdio>
#include <io.h>

#pragma comment (lib, "dbghelp.lib")

#include "msw_app_console_init.h"
#include "StringUtil.h"
#include "EnvironUtil.h"

MASTER_DEBUGGABLE

// CALL_FIRST means call this exception handler first;
// CALL_LAST means call this exception handler last
static const int CALL_FIRST		= 1;
static const int CALL_LAST		= 0;

const int MAX_APP_NAME_LEN = 24;
static bool s_unattended_session = 0;
static bool s_dump_on_abort = 1;

void msw_set_abort_message(bool onoff) {
	// _set_abort_behavior() only does things in debug CRT. We need popups in release builds too, so
	// we have to manage all this ourselves anyway...
	_set_abort_behavior(onoff, _WRITE_ABORT_MSG);
	s_unattended_session = onoff;
}

void msw_set_abort_crashdump(bool onoff) {
	s_dump_on_abort = onoff;
}

static bool s_abort_handler_no_debug = false;

void msw_abort_no_debug() {
	s_abort_handler_no_debug = true;
	abort();
}

static char const* getBasenameFromArgv0() {
	char const* basename = strrchr(__argv[0], '/');
	if (!basename) basename = __argv[0];
	basename += basename[0] == '/';
	return basename;
}

void msw_WriteFullDump(EXCEPTION_POINTERS* pep, const char* dumpname) {
	if (!dumpname) {
		dumpname = getBasenameFromArgv0();
	}

	DWORD Flags = MiniDumpNormal;

	Flags |= MiniDumpWithDataSegs;
	Flags |= MiniDumpWithFullMemoryInfo;
	if (pep) Flags |= MiniDumpWithThreadInfo;

	//Flags |= MiniDumpWithFullMemory;

	MINIDUMP_EXCEPTION_INFORMATION mdei;

	mdei.ThreadId           = ::GetCurrentThreadId();
	mdei.ExceptionPointers  = pep;
	mdei.ClientPointers     = FALSE;

	MINIDUMP_TYPE mdt       = MiniDumpNormal;

	char dumpname_bare[MAX_PATH];
	size_t length = strlen(dumpname);
	if (length >= 4 && length < MAX_PATH && !strcmp(dumpname + length - 4, ".exe"))
	{
		if (strcpy_s(dumpname_bare, dumpname))
			return;

		dumpname_bare[length - 4] = 0;
		dumpname = dumpname_bare;
	}

	char dumpfile[MAX_PATH];
	snprintf(dumpfile, "%.*s-crash.dmp", MAX_APP_NAME_LEN, dumpname);

	if (!dumpfile[0]) {
		// the filename is too long, a dump cannot be written (this should be unreachable since we're just writing
		// the dump into the CWD)
		return;
	}

	// GENERIC_WRITE, FILE_SHARE_READ used to minimize vectors for failure.
	// I've had multiple issues of this crap call failing for some permission denied reason. --jstine
	if (HANDLE hFile = CreateFileA(dumpfile, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr); hFile != INVALID_HANDLE_VALUE) {
		BOOL Result = MiniDumpWriteDump(
			::GetCurrentProcess(),
			::GetCurrentProcessId(),
			hFile,
			(MINIDUMP_TYPE)Flags,
			pep ? &mdei : nullptr,
			nullptr,
			nullptr
		);

		CloseHandle(hFile);

		if (!Result) {
			fprintf(stderr, "Minidump generation failed: %08x\n", ::GetLastError());
		}
		else {
			fprintf(stderr, "Crashdump written to %s\n", dumpfile);
		}
	}
}

static bool checkIfUnattended() {
	return s_unattended_session;
}

void msw_ShowCrashDialog(char const* title, char const* body) {
	if (checkIfUnattended()) {
		return;
	}

	// surface it to the user if no debugger attached, if debugger it likely already popped up an exception dialog...
	char fmt_buf[MAX_APP_NAME_LEN + 96];	// avoid heap, it could be corrupt
	snprintf(fmt_buf, "%s - %.*s", title, MAX_APP_NAME_LEN, getBasenameFromArgv0());
	auto result = ::MessageBoxA(nullptr, body, fmt_buf, MB_ICONEXCLAMATION | MB_OK);
}

static LONG NTAPI msw_BreakpointExceptionFilter(EXCEPTION_POINTERS* eps)
{
	// don't handle at all if there is a debugger attached.
	if (::IsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;

	if (eps->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT)
		return EXCEPTION_CONTINUE_SEARCH;

	fprintf(stderr, "Breakpoint at %p\n", eps->ExceptionRecord->ExceptionAddress);
	fflush(nullptr);

	msw_WriteFullDump(eps, nullptr);

	if (!::IsDebuggerPresent()) {
		msw_ShowCrashDialog("SIGSEGV", 
			"DebugBreak encountered.\n"
			"Check the console output for details."
		);
	}

	// pass exception to OS.
	return EXCEPTION_CONTINUE_SEARCH;
}

static LONG NTAPI msw_PageFaultExceptionFilter( EXCEPTION_POINTERS* eps )
{
	fflush(nullptr);

	if(eps->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
		return EXCEPTION_CONTINUE_SEARCH;

	// MS hides the target address of the load/store operation in the second
	// parameter in ExceptionInformation.

	void* access_address = (void*)eps->ExceptionRecord->ExceptionInformation[1];
	bool isWrite         = eps->ExceptionRecord->ExceptionInformation[0];

	fprintf(stderr, "PageFault %s at %p\n", isWrite ? "write" : "read", access_address);
	fflush(nullptr);

	msw_WriteFullDump(eps, nullptr);

	if (!::IsDebuggerPresent()) {
		msw_ShowCrashDialog("SIGSEGV", 
			"ACCESS VIOLATION (SIGSEGV) encountered.\n"
			"Check the console output for details."
		);
	}

	// pass pagefault to OS, it'll add an entry to the pointlessly over-engineered Windows Event Viewer.
	return EXCEPTION_CONTINUE_SEARCH;
}

void SignalHandler(int signal)
{
	if (signal == SIGABRT) {
		if (!::IsDebuggerPresent()) {
			if (s_dump_on_abort) {
				msw_WriteFullDump(nullptr, nullptr);
			}
			msw_ShowCrashDialog("ABORT", 
				"An error has occured and the application has aborted.\n"
				"Check the console output for details."
			);
		}

		// can't just return out because of a bug in Debug Static CRT that incorrectly deconstructs TLS.
		// (abort implciitly calls _Exit() internally after returning from this handler)
		exitNoCleanup(SIGABRT);
	}
}

void msw_InitAbortBehavior() {
	static bool bRun = 0;
	if (bRun) return;
	bRun = 1;

	// the crashdump filenameing logic depends on backslash replacement.
	for(char* p = __argv[0]; *p; ++p) {
		if (*p == '\\') *p = '/';
	}

	// Win10 no longer pops up msgs when exceptions occur. We'll need to produce crash dumps ourself...
	AddVectoredExceptionHandler(CALL_FIRST, msw_BreakpointExceptionFilter);
	AddVectoredExceptionHandler(CALL_FIRST, msw_PageFaultExceptionFilter);

	// let's not have the app ask QA to send reports to microsoft when an abort() occurs.
	_set_abort_behavior(0, _CALL_REPORTFAULT);

#if !defined(_DEBUG)

	typedef void (*SignalHandlerPointer)(int);
	auto previousHandler = signal(SIGABRT, SignalHandler);
#endif

	// Tell the system not to display the critical-error-handler message box.
	// from msdn: Best practice is that all applications call the process-wide SetErrorMode function
	//     with a parameter of SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs
	//     from hanging the application.
	//
	// Translation: disable this silly ill-advised legacy behavior. --jstine
	// (note in Win10, popups are disabled by default and cannot be re-enabled anyway)
	::SetErrorMode(SEM_FAILCRITICALERRORS);

#if ENABLE_ATTACH_TO_DEBUGGER
	if (!::IsDebuggerPresent()) {
		auto const* env_attach = getenv(g_env_attach_on_start);
		if (env_attach && env_attach[0] && env_attach[0] != '0') {
			s_attached_at_startup = spawnAtdExeAndWait();
		}
	}
#endif
}

static void modehack(DWORD handle, bool allocated) {
	DWORD mode;
	if (auto allocout = ::GetStdHandle(handle)) {
		GetConsoleMode(allocout, &mode);
		if (handle == STD_INPUT_HANDLE) {
			// if we turn these on when attaching to an existing console, it can totally mess up cmd.exe after the process exits.
			// (and there's no way we can safely undo this when the app is closed)
			if (allocated) {
				mode |= ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT;
			}
		}
		else {
			mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_LVB_GRID_WORLDWIDE;
		}
		SetConsoleMode(allocout, mode);
	}
};

bool msw_AttachConsole(int pid) {
	auto previn  = ::GetStdHandle(STD_INPUT_HANDLE );
	auto prevout = ::GetStdHandle(STD_OUTPUT_HANDLE);
	auto preverr = ::GetStdHandle(STD_ERROR_HANDLE );

	if (!prevout || !previn) {
		if (::AttachConsole(pid)) {
			modehack(STD_INPUT_HANDLE , false);
			modehack(STD_OUTPUT_HANDLE, false);
			modehack(STD_ERROR_HANDLE , false);

			// remap standard pipes to a newly-opened console.
			// _fi_redirect internally will retain the existing pipes when the application was opened, so that
			// redirections and original TTY/console will also continue to work.

			_fi_redirect_winconsole_handle(stdin,  previn );
			_fi_redirect_winconsole_handle(stdout, prevout);
			_fi_redirect_winconsole_handle(stderr, preverr);

			return 1;
		}
	}
	return 0;
}

void msw_AllocConsoleForWindowedApp() {
	// In order to have a windowed mode application behave in a normal way when run from an existing
	// _Windows 10 console shell_, we have to do this. This is because CMD.exe inside a windows console
	// doesn't set up its own stdout pipes, causing GetStdHandle(STD_OUTPUT_HANDLE) to return nullptr.
	//
	// this workaround is for attempting to recover the stdout from an app built as a Windowed application,
	// which normally gets piped to oblivion. This workaround won't capture stdout from DLLs though. Nothing
	// we can do about that. The DLL has to do its own freopen() calls on its own. Sorry folks.
	//
	// MINTTY: This problem does not occur on mintty or other conemu or other non-shitty console apps.
	// And we must take care _not_ to override their own pipe redirection bindings. This is why we only
	// call AttachConsole() if the stdio handle is NULL.

	auto previn  = ::GetStdHandle(STD_INPUT_HANDLE );
	auto prevout = ::GetStdHandle(STD_OUTPUT_HANDLE);
	auto preverr = ::GetStdHandle(STD_ERROR_HANDLE );

	if (::AllocConsole()) {
		// opt into the new VT100 system supported by Console and Terminal.

		modehack(STD_INPUT_HANDLE , true);
		modehack(STD_OUTPUT_HANDLE, true);
		modehack(STD_ERROR_HANDLE , true);
	}

	// remap standard pipes to a newly-opened console.
	// _fi_redirect internally will retain the existing pipes when the application was opened, so that
	// redirections and original TTY/console will also continue to work.

	_fi_redirect_winconsole_handle(stdin,  previn );
	_fi_redirect_winconsole_handle(stdout, prevout);
	_fi_redirect_winconsole_handle(stderr, preverr);
}

void msw_SetConsoleCP() {
	::SetConsoleCP(CP_UTF8);
	::SetConsoleOutputCP(CP_UTF8);

	modehack(STD_OUTPUT_HANDLE, false);
	modehack(STD_ERROR_HANDLE , false);
}

#if !defined(exitNoCleanup)
void exitNoCleanup(int exit_code) {
	// can't call _Exit() because of a bug in Debug Static MSCRT that incorrectly deconstructs TLS.
	// Even when using  Release MSCRT _Exit() isn't what we want: it sometimes call ExitProcess instead
	// of TerminateProcess, which also deconstructs threads (sigh). So TerminateProcess it is. 

	fflush(nullptr);
	::TerminateProcess(::GetCurrentProcess(), exit_code);
}
#endif

#endif
