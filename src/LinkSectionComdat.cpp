
#include <cstdio>
#include <posix_file.h>
#include "LinkSectionComdat.h"

#if !defined(PRESERVE_COMDAT_VIA_POSIX_WRITE)
#	if PLATFORM_MSW
#		define PRESERVE_COMDAT_VIA_POSIX_WRITE		0
#	else
#		define PRESERVE_COMDAT_VIA_POSIX_WRITE		1
#	endif
#endif

__noinline
void ForceSymbolToBeLinked::_ForceSymbolToBeLinked(const void* p) {

	// OK, so we have a __noinline directive but clang optimizes it away regardless. It doesn't inline the
	// function but it seems to inspect the function internals and optimize the codegen anyway?

	// doesn't work. Clang probably too smart and knows we never read the value back so it optimizes away.
	//static volatile std::atomic<int> force_side_effects;
	//force_side_effects = (intptr_t((void*)p));

	// [jstine 2022] Either of these work. I'm preferring posix_write because write(-1) is always a no-op
	//   while printf %.0s I'm less sure about. Note these only work because we're passing *p into an opaque
	//   function, meaning the function is implemented wholly in a DLL or PRX that the compiler cannot see.
#if PRESERVE_COMDAT_VIA_POSIX_WRITE
	// posix write works well on sony platforms but asserts on windows (annoyingly).
	posix_write(-1, p, 0);
#else
	// [jstine 2022] static atomic works on MSVC, but so does an empty printf. I'm not sure which I prefer.
	//static std::atomic<int> force_side_effects;
	//force_side_effects = (intptr_t((void*)p));
	printf("%.0s", p);
#endif

}
