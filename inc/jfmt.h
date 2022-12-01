#pragma once

// JFMT - printf formatting helper to solve variable int size problem.
//
// This promotes incoming scalar to either signed or unsigned intmax_t, in a similar way that C internally
// promotes float to double when passed into varadic arguments. Use the "%jd" or "%ju" pritnf formatters and
// and the compiler can do static analysis checks to verify the sign/unsigned nomenclature is matched.
//
// This is based on the same idea of how C compiler handles float/double internally already:
// that all float parameters are automatically promoted to double format when specified into
// va-args functions. I hope someday the C standard can somehow find a way to embrace the idea of
// doing the same for ints. If I want optimized codepaths, I'm not using va-args anyway. So let's
// just pick a size and make all parameters match it already. --jstine

__nodebug static inline auto JFMT(const int8_t &  scalar) { return intmax_t(scalar); }
__nodebug static inline auto JFMT(const int16_t&  scalar) { return intmax_t(scalar); }
__nodebug static inline auto JFMT(const int32_t&  scalar) { return intmax_t(scalar); }
__nodebug static inline auto JFMT(const int64_t&  scalar) { return intmax_t(scalar); }

__nodebug static inline auto JFMT(const uint8_t & scalar) { return uintmax_t(scalar); }
__nodebug static inline auto JFMT(const uint16_t& scalar) { return uintmax_t(scalar); }
__nodebug static inline auto JFMT(const uint32_t& scalar) { return uintmax_t(scalar); }
__nodebug static inline auto JFMT(const uint64_t& scalar) { return uintmax_t(scalar); }

__nodebug static inline int32_t  FMT64HI(int64_t  src) { return src >> 32; }
__nodebug static inline uint32_t FMT64LO(int64_t  src) { return src & ((1ULL<<32)-1); }
__nodebug static inline int32_t  FMT64HI(uint64_t src) { return src >> 32; }
__nodebug static inline uint32_t FMT64LO(uint64_t src) { return src & ((1ULL<<32)-1); }

// for use populating hex formatters like "%08x_%08x"
#define FMT64HILO(val) FMT64HI(val), FMT64LO(val)

struct StringViewTempArg;

// Custom string to integer conversions: sj for intmax_t, uj for uintmax_t. Recommendation is to
// always use these in splace of strtol / strtoll and then truncate or saturate to intended target
// value.
// 
// Supports binary notation (0b and 0B) and also C++14 literal separators:
//     0b1111'1011'0001
// 
// Supports implicit conversion from std::string and std::string_view (must include StdStringArg.h).
//
// Sets errno = ERANGE on truncated 64-bit input value and returns UINTMAX_MAX / INTMAX_MAX / INTMAX_MIN.

#include <string_view>
extern intmax_t  strtosj(char const* src,       char** endptr=nullptr, int radix=0);
extern intmax_t  strtosj(std::string_view src, char** endptr=nullptr, int radix=0);
extern uintmax_t strtouj(char const* src,       char** endptr=nullptr, int radix=0);
extern uintmax_t strtouj(std::string_view src, char** endptr=nullptr, int radix=0);

extern intmax_t  strtosj(char const* src,       int radix);
extern intmax_t  strtosj(std::string_view src, int radix);
extern uintmax_t strtouj(char const* src,       int radix);
extern uintmax_t strtouj(std::string_view src, int radix);
