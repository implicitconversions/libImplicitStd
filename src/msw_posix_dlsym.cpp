
#include "posix_dlsym.h"
#include "icy_assert.h"

#if PLATFORM_MSW

#include <Windows.h>

intptr_t posix_dlopen(const fs::path& origpath, uint32_t RTLD_flags) {

	// RTLD_flags is a posic specific thing that we ignore on windows.

	auto path = origpath;
	if (path.extension().empty())
		path += ".dll";

	auto handle = ::LoadLibraryA(path.c_str());
	if (!handle) {
		auto err = ::GetLastError();
		// how does windows have three possible errors for not finding a file? This makes no sense, as usual.
		// whatever plausible justification for this is a bad one. --jstine
		if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND || err == ERROR_MOD_NOT_FOUND) {
			return 0;
		}
		app_abort("LoadLibraryA(%s) failed, windows err 0x%08jx", path.c_str(), JFMT(err));
	}
	else {
		errprintf("posix_dlopen(%s) opened as dlhandle=%jx\n", path.c_str(), JFMT((intptr_t)handle));
	}

	return (intptr_t)handle;
}

void* posix_dlsym(intptr_t dlhandle, const char* name) {
	assertD(dlhandle);
	auto* result = ::GetProcAddress((HMODULE) dlhandle, name);
	if (!result) {
		auto err = ::GetLastError();
		if(err != ERROR_PROC_NOT_FOUND) {
			errprintf("GetProcAddress(dlhandle=%jx, '%s') failed, windows err 0x%08jx\n", JFMT(dlhandle), name, JFMT(err));
		}
	}
	return (void*)result;
}

void* posix_dlsymreq(intptr_t dlhandle, const char* name) {
	assertD(dlhandle);

	auto* result = ::GetProcAddress((HMODULE) dlhandle, name);
	if (!result) {
		auto err = ::GetLastError();
		if(err == ERROR_PROC_NOT_FOUND) {
			app_abort("posix_dlsymreq(dlhandle=%jx, '%s') required symbol not found.", JFMT(dlhandle), name);
		}
		else {
			app_abort("GetProcAddress(dlhandle=%jx, '%s') failed, windows err 0x%08jx", JFMT(dlhandle), name, JFMT(err));
		}
	}
	return (void*)result;
}

bool posix_dlclose(intptr_t dlhandle) {
	if (!dlhandle) return false;
	return ::FreeLibrary((HMODULE)dlhandle);
}

#endif
