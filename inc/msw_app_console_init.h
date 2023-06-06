#pragma once

#if PLATFORM_MSW

static constexpr int kMswAttachParentProcess = -1;

// specify a process or kMswAttachParentProcess
bool msw_AttachConsole(int pid);
void msw_AllocConsoleForWindowedApp();
void msw_set_abort_message(bool onoff);
void msw_set_abort_crashdump(bool onoff);
void msw_abort_no_debug();

// the following functions should already get called pre-main automatically.
// exposed here for sake of documentation and troubleshooting purpose.
void msw_SetConsoleCP();
void msw_InitAbortBehavior();

//void msw_WriteFullDump(EXCEPTION_POINTERS* pep, const char* dumpname);

#endif
