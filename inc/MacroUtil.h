#pragma once

#define ICY_EVAL(s) s
#define ICY_STRINGIZE(s) #s


// Strips parenthesis, such that (12345u) becomes 12345u
#define STRIP_PARENS(...) _IMPL_STRIP_PARENS __VA_ARGS__
#define _IMPL_STRIP_PARENS(...) __VA_ARGS__

#define _ICY_CONCAT_(l,r) l##r
#define ICY_CONCAT(l,r) _ICY_CONCAT_(l,r)

#define _IMPL_AUTO_ANON(line) auto ICY_CONCAT(_auto_anonymous_, line) =
#define AUTO_ANON _IMPL_AUTO_ANON(__LINE__)

// TODO: find a better home for this. For the moment it's kind of a one-off thing in icystdlib.
#if PLATFORM_MSW
	extern void exitNoCleanup(int exit_code);
#else
#	define exitNoCleanup(exit_code) (fflush(nullptr), _Exit(exit_code))
#endif
