#pragma once

#if __cplusplus
// quick workaround for older compilers not supporting std::ssize yet (c++20 limitation).
// Include <array> unconditionally to avoid unexpected build errors when compiling between C++17
// and C++20 feature sets.
#include <array>

// compilers don't support __has_cpp_attribute() yet, really. Get bizarre errors on both gcc and clang.
// will need to define HAS_STD_SSIZE manually via makefiles for anything that doesn't support it still.
#if !defined(HAS_STD_SSIZE)
#	define HAS_STD_SSIZE 	1
#endif

#if !HAS_STD_SSIZE
namespace std {
	template<class T>
	constexpr ptrdiff_t ssize(T const& obj) {
		return std::size(obj);
	}

	template<class T, ptrdiff_t N>
	constexpr ptrdiff_t ssize( const T (&array)[N] ) {
		return std::size(array);
	}
}
#endif
#endif
