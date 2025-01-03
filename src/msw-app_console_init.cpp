// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

#if PLATFORM_MSW
#ifndef NOMINMAX
	#define NOMINMAX 1
#endif
#include <Windows.h>
#include <Dbghelp.h>
#include <signal.h>
#include <cstring>
#include <cstdio>
#include <process.h>		// for spawnl/atd
#include <io.h>
#include <atomic>

#pragma comment (lib, "dbghelp")

#include "msw-app-console-init.h"
#include "StringUtil.h"
#include "EnvironUtil.h"
#include "defer.h"

TU_DEBUGGABLE

// TODO: separate all ATD logic into its own file and then allow programs to register the behavior
//       by some means, to avoid the need to recompile libimplicitstd with or without this feature.
#if !defined(WANT_ATTACH_TO_DEBUGGER)
#	define WANT_ATTACH_TO_DEBUGGER	(0)
#endif

// CALL_FIRST means call this exception handler first;
// CALL_LAST means call this exception handler last
static const int CALL_FIRST		= 1;
static const int CALL_LAST		= 0;

const int MAX_APP_NAME_LEN = 24;
static bool s_unattended_session = 0;
static bool s_dump_on_abort = 1;
static bool s_attach_to_debugger_on_exception = 1;

static bool s_abort_msg_is_set = 0;
static bool s_unattended_session_is_set = 0;
static bool s_attached_at_startup = 0;


char const* const g_atd_path_default      = "c:\\AttachToDebugger";
char const* const g_env_verbose_name      = "VERBOSE";
char const* const g_env_attach_debugger   = "ATTACH_DEBUG";
char const* const g_env_wait_debugger     = "WAIT_DEBUG";

extern bool check_boolean_semi_safe(char const* rvalue, bool defaultNonExist=false);

// abort message popup is typically skipped when debugger is attached. When not attached its purpose is to
// allow a debugger to attach, or to allow a user to ignore assertions and "hope for the best".
void msw_set_abort_message(bool onoff) {
	// _set_abort_behavior() only does things in debug CRT. We need popups in release builds too, so
	// we have to manage all this ourselves anyway...
	_set_abort_behavior(onoff, _WRITE_ABORT_MSG);
	s_abort_msg_is_set = 1;
}

void msw_set_unattended_mode(bool onoff) {
	s_unattended_session = onoff;
	s_unattended_session_is_set = 1;
}

void msw_set_auto_attach_debug(bool onoff) {
	s_attach_to_debugger_on_exception = onoff;
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

static DWORD volatile s_coredumps_in_flight;

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
	// We've had multiple issues of this crap call failing for some permission denied reason.
	InterlockedIncrement(&s_coredumps_in_flight);
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
			fprintf(stderr, "Minidump generation failed: %08jx\n", JFMT(::GetLastError()));
		}
		else {
			fprintf(stderr, "Crashdump written to %s\n", dumpfile);
		}

		fflush(nullptr);
		InterlockedDecrement(&s_coredumps_in_flight);
	}
}

static bool _mswFileExists(char const* path) {
	DWORD dwAttrib = GetFileAttributes(path);

	return (
			(dwAttrib != INVALID_FILE_ATTRIBUTES)
		&& !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
	);
}

static bool checkIfUnattended() {
	if (s_unattended_session_is_set) {
		return s_unattended_session;
	}

	return DiscoverUnattendedSessionFromEnvironment();
}

void msw_ShowCrashDialog(char const* title, char const* body) {
	if (checkIfUnattended()) {
		return;
	}

	// use a simple imperfect guard to avoid issuing multiple popups if many threads crash at the same time.
	// Secondary threads spin on the atomic until the first releases, to avoid preemptively crashing the app
	// by invoking EXCEPTION_CONTINUE_SEARCH while dialog for the first thread is shown.

	static std::atomic_bool s_dlg_shown;
	bool expected = false;

	if (s_dlg_shown.compare_exchange_strong(expected, true)) {
		// surface it to the user if no debugger attached, if debugger it likely already popped up an exception dialog...
		char fmt_buf[MAX_APP_NAME_LEN + 96];	// avoid heap, it could be corrupt
		snprintf(fmt_buf, "%s - %.*s", title, MAX_APP_NAME_LEN, getBasenameFromArgv0());
		auto result = ::MessageBoxA(nullptr, body, fmt_buf, MB_ICONEXCLAMATION | MB_OK);
		s_dlg_shown = false;
	}
	else {
		// don't print description, it's not useful here.
		errprintf("Async %s occured while waiting for crash dialog. Holding thread until dialog is retired.\n", title);
		while (s_dlg_shown) { Sleep(5); }
	}
}

static bool checkIfVerbose() {
	return check_boolean_semi_safe(getenv(g_env_verbose_name));
}

static void writeFullDumpGuarded(char const* excname, EXCEPTION_POINTERS* pep, const char* dumpname) {
	static std::atomic_bool s_once_coredump_written;
	bool expected = false;
	if (s_once_coredump_written.compare_exchange_strong(expected, true)) {
		msw_WriteFullDump(pep, nullptr);
	}
}

// If a crash/abort occurs while a coredump is being written, hold here to allow it to finish.
// Timeout after ~10 mins, because stuck processes are annoying.
static void holdThreadUntilDumpFinishes() {
	int sleep_slice_ms = 50;
	int timeout = (10*60*1000) / sleep_slice_ms;
	while (s_coredumps_in_flight > 0) {
		::Sleep(sleep_slice_ms);
		if (--timeout < 0) {
			errprintf("timed out waiting for coredump to complete");
			s_coredumps_in_flight = 0;		// force release all threads.
			break;
		}
	}
}

// Only returns TRUE if atd succeeded and debugger is verified as attached.
static bool spawnAtdExeAndWait() {
#if WANT_ATTACH_TO_DEBUGGER
	bool verbose = checkIfVerbose();

	static std::atomic_bool s_attach_excl;
	bool expected = false;
	if (!s_attach_excl.compare_exchange_strong(expected, true)) {
		while (s_attach_excl) { Sleep(5); }
		return ::IsDebuggerPresent();
	}

	char procstr[16];
	auto pid = ::GetCurrentProcessId();
	snprintf(procstr, "%jd", JFMT(pid));

	auto exitcode = 0;
	char atd_exe_path[MAX_PATH] = {};

	auto* bin_path = getenv("ATD_BIN_PATH");

	if (verbose && bin_path && bin_path[0]) {
		errprintf("(AttachToDebugger) ATD_BIN_PATH = %s\n", bin_path);
	}

	snprintf(atd_exe_path, "%s\\%s", bin_path ? bin_path : g_atd_path_default, "atd.exe");
	if (_mswFileExists(atd_exe_path)) {
		if (verbose) {
			errprintf("(AttachToDebugger) explicit atd.exe found = %s\n", atd_exe_path);
		}
		exitcode = _spawnl(_P_WAIT, atd_exe_path, atd_exe_path, procstr, nullptr);
	}
	else {
		if (verbose) {
			errprintf("(AttachToDebugger) trying to run atd.exe via PATH...\n");
		}
		exitcode = _spawnlp(_P_WAIT, "atd.exe", "atd.exe", procstr, nullptr);
	}

	if (exitcode) {
		if (exitcode < 0) {
			auto err = errno;
			errprintf("(AttachToDebugger) failed to spawn atd.exe: %s (%d)\n", strerror(err), err);
		}
		else {
			errprintf("(AttachToDebugger) atd.exe error code %d\n", exitcode);
		}

		if (!verbose) {
			errprintf("Run with %s=1 to log diagnostic information\n", g_env_verbose_name);
		}

		return false;
	}

	errprintf("Debugger requested for pid=%s, waiting for debugger to attach... ", procstr);
	fflush(nullptr);
	::Sleep(250);

	while (!::IsDebuggerPresent()) {
		::Sleep(60);
	}
	errprintf("Attached!\n");
	return true;
#endif

	return false;
}

static void checkAndSetCrtAbortBehavior() {
	if (s_abort_msg_is_set) {
		return;
	}

	bool shouldShowAbort = !checkIfUnattended() && !_isatty(fileno(stdin));
	_set_abort_behavior(shouldShowAbort, _WRITE_ABORT_MSG);
}

static LONG NTAPI msw_PageFaultExceptionFilter( EXCEPTION_POINTERS* eps )
{
	Defer(
		checkAndSetCrtAbortBehavior();
		holdThreadUntilDumpFinishes();
	);

	char const* excname = nullptr;
	switch (eps->ExceptionRecord->ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION	   : excname = "SIGSEGV";					break;
		case EXCEPTION_BREAKPOINT		   : excname = "Break/Stop Instruction";	break;
		case EXCEPTION_PRIV_INSTRUCTION	   : excname = "Privileged Instruction";	break;
		case EXCEPTION_ILLEGAL_INSTRUCTION : excname = "Illegal Instruction";		break;
		case EXCEPTION_STACK_OVERFLOW      : excname = "Stack Overflow";			break;
		case EXCEPTION_GUARD_PAGE	       : excname = "Guard Page Violation";		break;
	}

	//errprintf("(%s) Crash info!\n", excname);

	if (!excname) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	fflush(nullptr);

	bool verbose = checkIfVerbose();
	bool isUnattended = checkIfUnattended();


	if (eps->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
		// MS hides the target address of the load/store operation in the second
		// parameter in ExceptionInformation.
		void* access_address = (void*)eps->ExceptionRecord->ExceptionInformation[1];
		bool isWrite         = eps->ExceptionRecord->ExceptionInformation[0];
		errprintf("PageFault %s at %p\n", isWrite ? "write" : "read", access_address);
		fflush(nullptr);
	}

	if (verbose) {
		errprintf("(%s) unattended mode = %d\n", excname, isUnattended);
	}

	if (!::IsDebuggerPresent()) {
		if (!isUnattended && s_attach_to_debugger_on_exception) {
			// this will end up re-attaching debugger if using attachment on startup and detaching while a
			// breakpoint is stopped in the debugger. This only happens for exceptions with "First-chance exception"
			// feature enabled in visual studio. There's not much we can do about that since first chance exceptions
			// avoid our exception handler entirely, meaning the application has no way of knowing when they happen.
			// The fix is to disable first-chance exceptions for all Win32 excetpions. They're useless anyway, and
			// interfere with advanced emulator designs that rely on pagefaults to track memory accesses (JITs, etc).
			if (spawnAtdExeAndWait()) {
				// Continuing execution from here will result in the debugger picking up the breakpoint
				// at exactly the expected place in the callstack.
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}

		writeFullDumpGuarded(excname, eps, nullptr);

		char message_body[1024];

		snprintf(message_body,
			"%s encountered. Application will be closed.\n"
			"Attach a debugger before hitting OK to debug the crash.\n"
			"Check the console output for more details.",
			excname
		);

		msw_ShowCrashDialog(excname, message_body);
	}


	// pass pagefault to OS, it'll add an entry to the pointlessly over-engineered Windows Event Viewer.
	// Also, if user attached a debugger while the messagebox was modal, this will hook into the debugger
	// for them (nifty!).

	holdThreadUntilDumpFinishes();
	checkAndSetCrtAbortBehavior();
	return EXCEPTION_CONTINUE_SEARCH;
}

void SignalHandler(int signal)
{
	if (signal == SIGABRT) {
		if (!s_abort_handler_no_debug) {
			if (!::IsDebuggerPresent()) {
				if (s_dump_on_abort) {
					writeFullDumpGuarded("ABORT", nullptr, nullptr);
				}
				msw_ShowCrashDialog("ABORT",
					"An error has occured and the application has aborted.\n"
					"Check the console output for details."
				);
			}
		}

		checkAndSetCrtAbortBehavior();
		holdThreadUntilDumpFinishes();

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
	AddVectoredExceptionHandler(CALL_FIRST, msw_PageFaultExceptionFilter);

	// let's not have the app ask QA to send reports to microsoft when an abort() occurs.
	// (_WRITE_ABORT_MSG is established later, according to unattended mode settings)
	_set_abort_behavior(0, _CALL_REPORTFAULT);

	// Tell the system not to display the critical-error-handler message box.
	// from msdn: Best practice is that all applications call the process-wide SetErrorMode function
	//     with a parameter of SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs
	//     from hanging the application.
	//
	// Translation: disable this silly ill-advised legacy behavior. --jstine
	// (note in Win10, popups are disabled by default and cannot be re-enabled anyway)
	::SetErrorMode(SEM_FAILCRITICALERRORS);

#if WANT_ATTACH_TO_DEBUGGER
	if (!::IsDebuggerPresent()) {
		if (check_boolean_semi_safe(getenv(g_env_attach_debugger))) {
			s_attached_at_startup = spawnAtdExeAndWait();
		}

		if (!s_attached_at_startup && check_boolean_semi_safe(getenv(g_env_wait_debugger))) {
			errprintf("*** WAITING FOR DEBUGGER...\n");
			while(!::IsDebuggerPresent()) { ::Sleep(60); }
			errprintf("*** DEBUGGER ATTACHED.\n");
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

// Sample code for setting the font for an allocated system console.
// Not recommended to use this because Windows OS will remember user settings per-application, and the
// settings you can control vis Windows Console Settings are better than us trying to allow users to
// control all those things through the emulator configuration.
static void init_system_console_font() {
	if (auto allocout = ::GetStdHandle(STD_OUTPUT_HANDLE)) {
		CONSOLE_FONT_INFOEX font = {};
		font.cbSize = sizeof(font);
		font.nFont = 12;
		font.dwFontSize.X = 11;
		font.dwFontSize.Y = 19;
		font.FontFamily = 54;
		font.FontWeight = 400;
		wcscpy(font.FaceName, L"Consolas");

		SetCurrentConsoleFontEx(allocout, FALSE, &font);
	}
}

// pass in -1 to attach to an existing console parent.
bool msw_AttachConsole(int pid) {
	auto previn  = ::GetStdHandle(STD_INPUT_HANDLE );
	auto prevout = ::GetStdHandle(STD_OUTPUT_HANDLE);
	auto preverr = ::GetStdHandle(STD_ERROR_HANDLE );

	// it's possible to differentiate between a pipe redirection and a direct console output using GetConsoleMode,
	// but the usefulness of this in practice is suspect: for example, a non-console pipe could be piped through tee
	// and then that output still feeds into to a terminal window anyway. In practice it's better to just assume any
	// non-NULL handle returned from GetStdHandle represents a suitable console or represents an intentional pipe
	// redirection that implies we shouldn't open or bind our own console.

	// DWORD mode = 0;
	// ::GetConsoleMode(previn, mode);
	// ::GetConsoleMode(prevout, mode);
	// ::GetConsoleMode(preverr, mode);

	if (previn && prevout && preverr) {
		return 0;
	}

	if (!::AttachConsole(pid)) {
		return 0;
	}

	modehack(STD_INPUT_HANDLE , false);
	modehack(STD_OUTPUT_HANDLE, false);
	modehack(STD_ERROR_HANDLE , false);

	// remap standard pipes to a newly-opened console.
	// _fi_redirect internally will retain the existing pipes when the application was opened, so that
	// redirections and original TTY/console will also continue to work.

	if (!previn)  _fi_redirect_winconsole_handle(stdin,  nullptr);
	if (!prevout) _fi_redirect_winconsole_handle(stdout, nullptr);
	if (!preverr) _fi_redirect_winconsole_handle(stderr, nullptr);

	return 1;
}

// force - set TRUE to open a system console window even if another one appears to be attached.
//         When the window is force-created it will override stdout/stderr pipes set up by calling
//         process or the shell.
void msw_AllocConsoleForWindowedApp(bool force) {
	// CMD.exe inside a windows console or windows terminal doesn't set up its own stdout pipes, causing
	// GetStdHandle(STD_OUTPUT_HANDLE) to return nullptr. This makes it look like no console is attached
	// even though one actually exists.

	// this will attach the app to a windows console/terminal if run from one.
	// if run from mintty or conemu, then this call will do nothing and return FALSE.
	if (msw_AttachConsole(-1)) {
		return;
	}

	auto previn  = ::GetStdHandle(STD_INPUT_HANDLE );
	auto prevout = ::GetStdHandle(STD_OUTPUT_HANDLE);
	auto preverr = ::GetStdHandle(STD_ERROR_HANDLE );

	// mintty/conemu will exit here since they will have set up prevout/preverr.
	// If stdin is null but stdout/stderr are valid, don't open a console unless "forced"
	if ((prevout && preverr) && !force) {
		return;
	}

	// getting this far means the user ran the app from Explorer or some other means where
	// there's no console window present -and- there's
	if (!::AllocConsole()) {
		return;
	}

	// opt into the new VT100 system supported by Console and Terminal.
	modehack(STD_INPUT_HANDLE , true);
	modehack(STD_OUTPUT_HANDLE, true);
	modehack(STD_ERROR_HANDLE , true);

	// remap standard pipes to a newly-opened console.
	// _fi_redirect internally will retain the existing pipes when the application was opened, so that
	// redirections and original TTY/console will also continue to work.

	// TODO: we could get fancy here and allow piping to a file while also showing output to the console.
	//       but that activity needs to be very intentionally prescribed by the user via CLI or something
	//       and for the moment _fi_redirect_winconsole_handle() is not well suited to that task.

	if (force || !previn)  _fi_redirect_winconsole_handle(stdin,  previn );
	if (force || !prevout) _fi_redirect_winconsole_handle(stdout, prevout);
	if (force || !preverr) _fi_redirect_winconsole_handle(stderr, preverr);

	// UTF8 now works in Windows System Console as well as Windows Terminal (Win11, 22H2)
	//printf( "UTF8 Test: これが方法です\n");
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
