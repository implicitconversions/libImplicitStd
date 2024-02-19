#pragma once

#if defined(__clang__)
// printf format validation should be errors rather than warnings. The only possible exception is -Wformat-extra-args which itself is a
// low-risk formatting mistake that doesn't cause runtime failures, eg. extra agrs are simply unused. But in the context of string literal
// formatters, there is no reason to allow them (their use is restricted to variable or templated constexpr formatters that might opt
// to ignore some args depending on context and those would not trigger this error anyway).

#	pragma GCC diagnostic error "-Wformat"
#	pragma GCC diagnostic error "-Wformat-extra-args"
#	pragma GCC diagnostic error "-Wformat-insufficient-args"
#	pragma GCC diagnostic error "-Wformat-invalid-specifier"
#	pragma GCC diagnostic error "-Winconsistent-missing-override"

#	pragma GCC diagnostic error "-Wreturn-type"					// non-void function does not return a value. Should have always been an error.
#	pragma GCC diagnostic error "-Wparentheses"					// catches important operator precedence issues. More() is better, usually.
#endif
