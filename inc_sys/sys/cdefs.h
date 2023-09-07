#pragma once

// GCC cdefs.h forcibly defines some things incorrectly so we have to override it here so
// that those things (__always_inline macro, for example) actually behave in a cross-platform
// behavioral manner. GCC provides `include_next` explicitly for this purpose (yay!)

#if !PLATFORM_MSW

#include_next <sys/cdefs.h>

// __always_inline: Linux kernel defines it correctly but gnu glibc defines it with `inline`
// baked in. This is wrong in a world with LTO compilation. It causes issues because `inline`
// (C++ directive) has very different purpose and characteristics from `always_inline` as a
// general compuler optimization directive. Put simply, there are times when we want always_inline
// behavior but do not want to also imply c++ `inline` for the current TU. This is most useful
// when paired with LTO features which allow performing full compiler optimization acrosss TUs,
// including the inlining of functions. --jstine

#undef __always_inline
#define __always_inline __attribute__ ((__always_inline__))

#endif
