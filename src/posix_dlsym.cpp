
#include "posix_dlsym.h"
#include "icy_assert.h"

#include <dlfcn.h>

#if PLATFORM_POSIX

intptr_t posix_dlopen(const fs::path& origpath, uint32_t RTLD_flags) {
	auto path = origpath;
	if (path.extension().empty())
		path += ".dll";

	return (intptr_t)dlopen(origpath.c_str(), RTLD_flags);
}

void* posix_dlsym(intptr_t dlhandle, const char* name) {
	return dlsym(dlhandle, name);
}

void* posix_dlsymreq(intptr_t dlhandle, const char* name) {
	auto result = dlsym((void*)dlhandle, name);
	if (!result) {
		assertR("posix_dlsymreq(dlhandle=%jx, '%s') required symbol not found.", JFMT(dlhandle), name);
	}
	return result;
}

bool posix_dlclose(intptr_t dlhandle) {
	return ::dlclose((void*)dlhandle);
}

#endif
