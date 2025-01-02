// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

#if PLATFORM_MSW
#include "EmbeddedBinaryData.h"
#include <Windows.h>

// Used to identify a DLL as well as an executable.
// Works as a more robust replacement for ::GetModuleHandle()
extern "C" IMAGE_DOS_HEADER __ImageBase;

std::tuple<void const*,intmax_t> msw_GetResourceInfo(std::string resource_name)
{
    HGLOBAL     res_handle = NULL;
    HRSRC       res;

	auto hModule = reinterpret_cast<HMODULE>(&__ImageBase);

    res = FindResource(hModule, resource_name.c_str(), RT_RCDATA);
    if (!res)
        return {};

    res_handle = LoadResource(hModule, res);
    if (!res_handle)
        return {};

	// LocResource doesn't actually lock anything - it's just a legacy API from Win95 era.
    return {
		LockResource(res_handle),
	    (intmax_t)SizeofResource(hModule, res)
	};
}
#endif
