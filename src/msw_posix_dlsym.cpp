
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
		app_abort("LoadLibraryA(%s) failed, windows err 0x%08x", path.c_str(), err);
	}
	else {
		errprintf("posix_dlopen(%s) opened as dlhandle=%jx\n", JFMT((intptr_t)handle));
	}

	return (intptr_t)handle;
}

void* posix_dlsym(intptr_t dlhandle, const char* name) {
	assertD(dlhandle);
	void* result = ::GetProcAddress((HMODULE) dlhandle, name);
	if (!result) {
		auto err = ::GetLastError();
		if(err != ERROR_PROC_NOT_FOUND) {
			errprintf("GetProcAddress(dlhandle=%jx, '%s') failed, windows err 0x%08x\n", JFMT(dlhandle), name, err);
		}
	}
	return result;
}

void* posix_dlsymreq(intptr_t dlhandle, const char* name) {
	assertD(dlhandle);

	void* result = ::GetProcAddress((HMODULE) dlhandle, name);
	if (!result) {
		auto err = ::GetLastError();
		if(err == ERROR_PROC_NOT_FOUND) {
			app_abort("posix_dlsymreq(dlhandle=%jx, '%s') required symbol not found.", JFMT(dlhandle), name);
		}
		else {
			app_abort("GetProcAddress(dlhandle=%jx, '%s') failed, windows err 0x%08x", JFMT(dlhandle), name, err);
		}
	}
	return result;
}

bool posix_dlclose(intptr_t dlhandle) {
	if (!dlhandle) return false;
	return ::FreeLibrary((HMODULE)dlhandle);
}

#endif
