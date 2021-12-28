
#if !!PLATFORM_MSW
#include <Windows.h>
#include <Dbghelp.h>
#include <signal.h>
#include <cstring>
#include <cstdio>
#include <corecrt_startup.h>

#pragma comment (lib, "dbghelp.lib")

#include "msw_app_console_init.h"

MASTER_DEBUGGABLE

// CALL_FIRST means call this exception handler first;
// CALL_LAST means call this exception handler last
static const int CALL_FIRST		= 1;
static const int CALL_LAST		= 0;

const int MAX_APP_NAME_LEN = 24;
static bool s_interactive_user_environ = 1;
static bool s_dump_on_abort = 1;

void msw_set_abort_message(bool onoff) {
	// _set_abort_behavior() only does things in debug CRT. We need popups in release builds too, so
	// we have to manage all this ourselves anyway...
	_set_abort_behavior(onoff, _WRITE_ABORT_MSG);
	s_interactive_user_environ = onoff;
}

void msw_set_abort_crashdump(bool onoff) {
	s_dump_on_abort = onoff;
}

void msw_WriteFullDump(EXCEPTION_POINTERS* pep, const char* dumpname)
{
	DWORD Flags = MiniDumpNormal;

	Flags |= MiniDumpWithDataSegs;
	Flags |= MiniDumpWithFullMemoryInfo;
	if (pep) Flags |= MiniDumpWithThreadInfo;

	//Flags |= MiniDumpWithFullMemory;

	MINIDUMP_EXCEPTION_INFORMATION mdei;

	mdei.ThreadId           = GetCurrentThreadId();
	mdei.ExceptionPointers  = pep;
	mdei.ClientPointers     = FALSE;

	MINIDUMP_TYPE mdt       = MiniDumpNormal;

	char dumpfile[240];
	sprintf_s(dumpfile, "%.*s-crash.dmp", MAX_APP_NAME_LEN, dumpname);

	// GENERIC_WRITE, FILE_SHARE_READ used to minimize vectors for failure.
	// I've had multiple issues of this crap call failing for some permission denied reason. --jstine
	if (HANDLE hFile = CreateFileA(dumpfile, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)) {
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

	char const* basename = strrchr(__argv[0], '/');
	if (!basename) basename = __argv[0];
	basename += basename[0] == '/';

	msw_WriteFullDump(eps, basename);

	if (s_interactive_user_environ && !::IsDebuggerPresent()) {
		// surface it to the user if no debugger attached, if debugger it likely already popped up an exception dialog...
		char fmt_buf[MAX_APP_NAME_LEN + 24];	// avoid heap, it could be corrupt
		sprintf_s(fmt_buf, "SIGSEGV - %.*s", MAX_APP_NAME_LEN, basename);
		auto result = ::MessageBoxA(nullptr,
			"ACCESS VIOLATION (SIGSEGV) has occurred and the process has been terminated. "
			"Check the console output for more details.",
			fmt_buf,
			MB_ICONEXCLAMATION | MB_OK
		);
	}

	// pass pagefault to OS, it'll add an entry to the pointlessly over-engineered Windows Event Viewer.
	return EXCEPTION_CONTINUE_SEARCH;
}

void SignalHandler(int signal)
{
	if (signal == SIGABRT) {
		if (!s_dump_on_abort) {
			return;
		}

		char const* basename = strrchr(__argv[0], '/');
		if (!basename) basename = __argv[0];
		basename += basename[0] == '/';

		msw_WriteFullDump(nullptr, basename);

		if (s_interactive_user_environ && !::IsDebuggerPresent()) {
			char fmt_buf[MAX_APP_NAME_LEN + 24];	// avoid heap.
			sprintf_s(fmt_buf, "abort() - %.*s", MAX_APP_NAME_LEN, basename);

			auto result = ::MessageBoxA(nullptr,
				"An error has occured and the application has aborted.\n"
				"Check the console output for details.",
				fmt_buf,
				MB_ICONEXCLAMATION | MB_OK
			);
		}
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
	_set_abort_behavior(0, _CALL_REPORTFAULT);

#if !defined(_DEBUG)

	typedef void (*SignalHandlerPointer)(int);
	auto previousHandler = signal(SIGABRT, SignalHandler);
#endif

	// abort message popup is typically skipped when debugger is attached. When not attached its purpose is to
	// allow a debugger to attach, or to allow a user to ignore assertions and "hope for the best".
	msw_set_abort_message(1);

	auto* automated_flag    = getenv("AUTOMATED");
	auto* noninteract_flag  = getenv("NONINTERACTIVE");
	auto* jenkins_node_name = getenv("NODE_NAME");
	auto* jenkins_job_name  = getenv("JOB_NAME");

	bool allow_popups = 1;

	// heuristically autodetect jenkins first, and honor explicit AUTOMATED/NONINTERACTIVE afterward.

	if ((jenkins_node_name && jenkins_node_name[0]) &&
		(jenkins_job_name  && jenkins_job_name [0])) {
		allow_popups = 0;
	}

	if (automated_flag  ) allow_popups   = (automated_flag  [0] != '0');
	if (noninteract_flag) allow_popups   = (noninteract_flag[0] != '0');

	msw_set_abort_message(allow_popups);

	// Tell the system not to display the critical-error-handler message box.
	// from msdn: Best practice is that all applications call the process-wide SetErrorMode function
	//     with a parameter of SEM_FAILCRITICALERRORS at startup. This is to prevent error mode dialogs
	//     from hanging the application.
	//
	// Translation: disable this silly ill-advised legacy behavior. --jstine
	// (note in Win10, popups are disabled by default and cannot be re-enabled anyway)
	::SetErrorMode(SEM_FAILCRITICALERRORS);

}

void msw_InitAppForConsole() {
	static bool bRun = 0;
	if (bRun) return;
	bRun = 1;

	msw_InitAbortBehavior();

	// In order to have a windowed mode application behave in a normal way when run from an existing
	// _Windows 10 console shell_, we have to do this. This is because CMD.exe inside a windows console
	// doesn't set up its own stdout pipes, causing GetStdHandle(STD_OUTPUT_HANDLE) to return nullptr.
	//
	// MINTTY: This problem does not occur on mintty or other conemu or other non-shitty console apps.
	// And we must take care _not_ to override their own pipe redirection bindings. This is why we only
	// call AttachConsole() if the stdio handle is NULL.

	auto previn  = ::GetStdHandle(STD_INPUT_HANDLE );
	auto prevout = ::GetStdHandle(STD_OUTPUT_HANDLE);
	auto preverr = ::GetStdHandle(STD_ERROR_HANDLE );

	if (!prevout && !previn) {
		// this workaround won't capture stdout from DLLs. Nothing we can do about that. The DLL has
		// to do its own freopen() calls on its own. Sorry folks.
		// easy solution: don't use cmd.exe or windows shitty terminal!

		bool console_attached = 0;
		bool console_created = 0;
		console_attached = ::AttachConsole(ATTACH_PARENT_PROCESS);

		if (!console_attached && s_interactive_user_environ) {
			// stdout is dangling, alloc windows built-in console to visualize it.
			// This is kind of an "emergency procedure only" option, since this console is
			// ugly, buggy, and generally limited.

			console_attached = console_created = ::AllocConsole();
		}

		if (!console_attached) {
			// Console could fail to allocate on a Jenkins environment, for example.
			// And in this specific case, the thing we definitely don't want to do is deadlock the
			// program waiting for input from a non-existant user, so guard it against the interactive_user
			// environment flag.
			if (s_interactive_user_environ) {
				char fmt_buf[256];	// avoid heap.
				sprintf_s(fmt_buf,
					"AllocConsole() failed, error=0x%08x\n"
					"To work-around this problem, please run the application from an existing console, "
					"by opening Git BASH, MinTTY, or Windows Command Prompt and starting the program there.\n",
					::GetLastError()
				);

				auto result = ::MessageBoxA(nullptr, fmt_buf,
					"AllocConsole Failure",
					MB_ICONEXCLAMATION | MB_OK
				);
			}
		}

		if (console_attached) {
			// opt into the new VT100 system supported by Console and Terminal.
			auto modehack = [](DWORD handle) {
				DWORD mode;
				if (auto allocout = ::GetStdHandle(handle)) {
					GetConsoleMode(allocout, &mode);
					if (handle == STD_INPUT_HANDLE) {
						// if we turn these on when attaching to an existing console, it can totally mess up cmd.exe after the process exits.
						// (and there's no way we can safely undo this when the app is closed)
						//mode |= ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT;
					}
					else {
						mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_LVB_GRID_WORLDWIDE;
					}
					SetConsoleMode(allocout, mode);
				}
			};

			modehack(STD_INPUT_HANDLE);
			modehack(STD_OUTPUT_HANDLE);
			modehack(STD_ERROR_HANDLE);

			msw_set_abort_message(0);
		}

		if (console_created) {
			// remap standard pipes to a newly-opened console.
			// _fi_redirect internally will retain the existing pipes when the application was opened, so that
			// redirections and original TTY/console will also continue to work.

			_fi_redirect_winconsole_handle(stdin,  previn );
			_fi_redirect_winconsole_handle(stdout, prevout);
			_fi_redirect_winconsole_handle(stderr, preverr);
		}
	}

	// when MSYS BASH shell is present somewhere, assume the user doesn't want or need popups.
	// (can be overridden by CLI switch in the main app, etc)
	if (getenv("SHLVL")) {
		msw_set_abort_message(0);
	}

	// Set console output to UTF8
	//  - This will work if the OS is using Windows Terminal instead of Windows Console.
	//  - This will work if the Windows Console is using a single font that supports all glyphs, but all such fonts
	//    are hideous and so it's mostly useless. :(

	::SetConsoleCP(CP_UTF8);
	::SetConsoleOutputCP(CP_UTF8);
}

static int _init_abort_behavior(void) {
	msw_InitAbortBehavior();
	return 0;
};

static int _init_console_behavior(void) {
	msw_InitAppForConsole();
	return 0;
};

// .CRT$XIC are the CRT C initializers, and anything run before those won't have stdio available.
//   This makes .CRT$XID a good spot for our own stuff to run as early as possible, and without crashing.
// (and I still can't get these to work reliably from a .lib file .. ? --jstine)

__pragma (section(".CRT$XIDA", read));
__pragma (section(".CRT$XIDB", read));

__declspec(allocate(".CRT$XIDA"))
static _PIFV init_abort_behavior = _init_abort_behavior;

__declspec(allocate(".CRT$XIDB"))
static _PIFV init_console_behavior = _init_console_behavior;

// LTCG will deadref these things, so we use lambdas to force persistent references.
// This works mainly because the lambda needs to be constructed at C++ initializer step in a way that
// makes it super unlikely that the compiler will ever optimize it away.

static bool deadref1 = []() { return init_abort_behavior; }();
static bool deadref2 = []() { return init_console_behavior; }();

#endif
