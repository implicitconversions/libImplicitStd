#pragma once

#if PLATFORM_MSW

void msw_InitAppForConsole();
void msw_set_abort_message(bool onoff);
void msw_set_abort_crashdump(bool onoff);
void msw_AllocWinNativeConsole();

//void msw_WriteFullDump(EXCEPTION_POINTERS* pep, const char* dumpname);

#endif
