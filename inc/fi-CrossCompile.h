#pragma once

// [jstine 2022] Workaround for CLANG not enabling the C++20 preprocessor feature when doing Intellisense
//    passes, even though the compiler appears to be using the C++20 conformant preprocessor. This hack has a drawback
//    that it produces invalud results for uses of __VA_OPT__ that are not comma-deletion (such as string pasting with
//    # __VA_ARGS__). But it's just intellisense so breaking a macro or two is no real problem. Part of this hack also
//    incorporates adding ## back in which are technically unnecessary in the C++20 preprocessor. Example:
//              __VA_OPT__(,) ## __VA_ARGS__
//                            ^^ all instances of this can be removed at the same time this hack is removed.
//
//    It would great if this workaround also worked for MSC99, but Microsoft's C99 compiler can't expand __VA_ARGS__
//    in a way that allows this trick to work. So anything we want to work from C code needs to have fully old-
//    fashioned VA_ARGS implementation in addition to VA_OPT implementation (sigh).

#if defined(__clang__) && __INTELLISENSE__ && _MSVC_TRADITIONAL
#	define __VA_OPT__(...)    __VA_ARGS__
#	define VA_OPT(...)	      __VA_ARGS__
#endif

// clang-cl egregiously defines both __clang__ and _MSC_VER even though it isn't conforming to MSC compiler builtin
// behaviors at all. This effectively ruins any meaning/use that _MSC_VER used to have. Workaround this by defining
// our own COMPILER_MSC flag, which can (and should) be used in place of _MSC_VER. Thanks, Microsoft.
#if defined(_MSC_VER)
#	if defined(__clang__)
#		define COMPILER_MSC (0)
#	else
#		define COMPILER_MSC (1)
#	endif
#endif

// C/C++ interop: nullptr and bool.
#if !__cplusplus
#	define EXTERN_C extern
#	define nullptr NULL
	typedef _Bool bool;
#else
#	define EXTERN_C extern "C"
#endif

// gcc/clang provide this define normally, and if not then assume any attribute we check for doesn't exist.
#if !defined(__has_attribute)
#	define __has_attribute(x)  (0)
#endif

#if !defined(LAMBDA_ATTRIBUTE)
#	if defined(COMPILER_MSC)
#		define LAMBDA_ATTRIBUTE(...)	__VA_ARGS__
#	elif defined(__clang__)
#		define LAMBDA_ATTRIBUTE(...)	__VA_ARGS__
#	else
#		define LAMBDA_ATTRIBUTE(...)
#	endif
#endif

// yesinline explained:
//   can't use __always_inline because glibc hijacked it and broke it by defining it as "inline __attribute__((__always_inline__))"
//   instead of just the attribute, thereby mixing two entirely different directives (one for the TU and one for the optimizer)
//   into one statement. This becomes a big problem when using LTO, which by deifnition allows us to do inline optimization
//   across modules without having to use the C++ `inline` keyword and include them into all TUs.

// Help MSVC Conform to the GCC/Clang way of doing things.
#if COMPILER_MSC

#	define __builtin_bswap32(a)			_byteswap_ulong(a)
#	define __builtin_unreachable()		__assume(0)
#	define __builtin_assume(cond)		__assume(cond)
#	define __builtin_trap()				__debugbreak()
#	define __used
#	define __immdebug					__declspec(dllexport)		// expose for VS immediate debug window use
#	define __retain_used				__declspec(dllexport)

#	if __cplusplus
#		define __unused						[[maybe_unused]]
#		define __debugvar					[[maybe_unused]]			// alias for __unused, indicates intent more clearly in some cases.
#	else
#		define __unused						// msc has no unused equivalnt in C or __Declspec
#		define __debugvar					// msc has no unused equivalnt in C or __Declspec
#	endif

#	if __cplusplus > 201704
#		define __msvc_always_inline			[[msvc::forceinline]]
#		define __msvc_ctor_inline			__forceinline		// workaround for VS2019 bug (https://developercommunity.visualstudio.com/t/msvc::forceinline-is-ignored-when-ap/1623437)
#	else
#		define __msvc_always_inline			// can't do __forceinline here, it's not safe, behavior differs from clang too much.
#		define __msvc_ctor_inline			__forceinline
#	endif

#	define __noinline					__declspec(noinline)
#	define yesinline					__msvc_always_inline
#	define __ctor_inline				__msvc_ctor_inline		// workaround for VS2019 bug (https://developercommunity.visualstudio.com/t/msvc::forceinline-is-ignored-when-ap/1623437)

#	define __va_inline 					__msvc_always_inline	// workaround for GCC complaining about always_inline on varadics

#else

#	define yesinline					__attribute__((__always_inline__))

#   define __ctor_inline				yesinline
#	define __immdebug					[[gnu::used, gnu::retain]]	// expose for VS immediate debug window use
#	define __retain_used				[[gnu::used, gnu::retain]]
#   define __unused                     [[maybe_unused]]
#	define __debugvar					[[maybe_unused]]			// alias for __unused, indicates intent more clearly in some cases.

#	if !PLATFORM_SCE
#		define __noinline 				__attribute__((__noinline__))
#	endif

#	if !defined(__MINGW64__)
#		define __debugbreak()			__builtin_debugtrap()
#	endif

#	if defined(__GNUC__)
		// workaround for GCC complaining about always_inline on varadics
		// - Why is this an error and not a warning like all other situations where yesinline fails to apply?
		// - Why can't it be turned off like other errors/warnings?
#		define __va_inline
#	else
#		define __va_inline 					yesinline
#	endif
#endif

// __builtin_expect is too clumsy to use as a regular keyword in practical use cases, so rather than define a
// version of it for Microsoft, define expect_true/expect_false macros instead.
// NOTE: The return value of __builtin_expect is the return value of the expression. Use explicit typecast to bool
//       to ensure the result is consistent acorss compilers.
#if COMPILER_MSC
#  define expect_true(expr)     ((bool)(expr))
#  define expect_false(expr)    ((bool)(expr))
#else
#  define expect_true(expr)     (__builtin_expect ((bool)(expr),true ) )
#  define expect_false(expr)    (__builtin_expect ((bool)(expr),false) )
#endif

// this macro allows the use of _Pragma with automatic escaping of quotes in strings, and is recommended by the GCC
// docs as the preferred way to work around the annoying design restrictions of _Pragma(). It essentially makes
// the C99 _Pragma feature behave just like the Microsoft __pragma feature, therefore I've named it accordingly
// and inject it only when compiling on a toolchain that lacks __pragma. This simplifies MSVC/Clang interop.
#if !COMPILER_MSC
#	define __pragma(x)     _Pragma (#x)
#endif

// Slightly less annoying way to deal with MSVC's lack of an equivalent to GCC/Clang __attribute__((packed)).
// Note that we do not need to specify alignas(1), but if we do it hints MSVC to produce compiler warnings
// anytime the packing isn't working the way we expect.
#if COMPILER_MSC
#	define __PACKED(...)  __pragma(pack(push,1)) __VA_ARGS__ alignas(1) __pragma(pack(pop))
#else
#   define __packed __attribute__((__packed__))
#	define __PACKED(...)  __VA_ARGS__ __packed
#endif

// This is a C++ pseudo-macro edition of C# using() idiom. USes C++11 if() construct to create a scoped variable
// within a code block that is always run. Use this to replace various if(1) idioms.
#define cpp_using(...) if (__VA_ARGS__; true)

#if defined(_M_X64_) || defined(_M_AMD64)
#	define __aligned_simd		__declspec(align(16))
#	define __cachelined			__declspec(align(64))
#else
#   define __aligned_simd
#   define __cachelined
#endif

#if COMPILER_MSC
#	define __selectany __declspec(selectany)
#else
#	define __selectany
#endif

#if COMPILER_MSC
#	define __verify_fmt(fmtpos, vapos)
#else
#	define __verify_fmt(fmtpos, vapos)  __attribute__ ((format (printf, fmtpos, vapos)))
#endif

// TU_DEBUGGABLE - TranslationUnit Debuggable.
//   Disables optimizations for the given TU. Intended for sue at the top of a file, but can also be
//   used in the middle of a file to allow the latter portion of the file to be debuggable.
//
// Implementation Remarks:
//   Ironically, trying to reduce warning spam on one compiler will cause added warning spam on
//   another compiler in the form of "unsupported pragma directive", so provide some helper macros
//   to expedite local-scope warning disabling without adding warings to other compilers.
#if COMPILER_MSC
#	define TU_DEBUGGABLE	__pragma(optimize("", off))
#elif defined(__clang__)
	// [jstine 2022] clang will spam warnings about ignored attributes around yesinline. We don't want to turn this
	// warning off globally because it has about 25 useful warnings in addition to this one very useless one.
#	define TU_DEBUGGABLE		\
		__pragma(clang optimize off)	\
		__pragma(clang diagnostic ignored "-Wignored-attributes")
#else
#	define TU_DEBUGGABLE		\
		__pragma(gcc optimize off)	\
		__pragma(gcc diagnostic ignored "-Wignored-attributes")
#endif

#if COMPILER_MSC
#	define MSC_WARNING_DISABLE(nums)			\
		__pragma(warning(disable: nums))

#	define MSC_WARNING_DISABLE_PUSH(nums)		\
		__pragma(warning(push))					\
		__pragma(warning(disable: nums))

#	define MSC_WARNING_DISABLE_POP()			\
		__pragma(warning(pop))
#	else
#	define MSC_WARNING_DISABLE(nums)
#	define MSC_WARNING_DISABLE_PUSH(nums)
#	define MSC_WARNING_DISABLE_POP()
#endif

#if defined(__clang__)
#	define CLANG_WARNING_DISABLE(args) __pragma(clang diagnostic ignored args)
#else
#	define CLANG_WARNING_DISABLE(args)
#endif

// for better or for worse. preprocessor has elif, might as well give it to the whole language. :)
#define elif	else if
