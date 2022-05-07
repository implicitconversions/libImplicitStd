
#include <cstdio>
#include <posix_file.h>
#include "LinkSectionComdat.h"

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

	posix_write(-1, p, 0);
	//printf("%.0s", p);
}
