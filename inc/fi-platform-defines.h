// Contents released under the The MIT License (MIT)

#pragma once

// the Microsoft CRT has a define that's been present, unconditionally, since its inception: '_CRT_PACKING'
// This name is also weird enough to be fairly safe for us to use as a detection clause.
// Special build requirements can always define PLATFORM_MSW explicitly from your makefile.


// Default selection of MSW platform is based on using the Microsoft Compiler.
// This is not a foolproof assumption, but a special environment can specify these
// defines explicitly via makefile.

#if !defined(PLATFORM_MSW)
#   if defined(_WIN32)
#       define PLATFORM_MSW     1
#   else
#       define PLATFORM_MSW     0
#   endif
#endif

#if !defined(PLATFORM_MSCRT)
#   if defined(_WIN32)
#       define PLATFORM_MSCRT   1
#   else
#       define PLATFORM_MSCRT   0
#   endif
#endif

#if !defined(PLATFORM_PS4)
#   if defined(__ORBIS__)
#       define PLATFORM_PS4     1
#   else
#       define PLATFORM_PS4     0
#   endif
#endif

#if !defined(PLATFORM_PS5)
#   define PLATFORM_PS5         0
#endif

#define PLATFORM_SCE (PLATFORM_PS4 || PLATFORM_PS5)

