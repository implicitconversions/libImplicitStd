
#include "LinkSectionComdat.h"

__nodebug __noinline
void ForceSymbolToBeLinked::_ForceSymbolToBeLinked(const void* p) {

	// another suitable option is to call some opaque function in a DLL that's assured not to be optimizable.
	// on Windows this could be SetLastError(). I am opting for std::atomic here since it avoids touching
	// anything that actually could matter (like errno), and the compiler barrier ensures things can't be
	// optimized away even if the compiler happens to know a whole lot about the implementation details of
	// whatever fnction we hope might be opaque.

	static std::atomic<int> force_side_effects;
	force_side_effects = (intptr_t((void*)p));
}
