#pragma once

// JFMT - printf formatting helper to solve variable int size problem.
//
// This upconverts incoming scalar to either signed or unsigned intmax_t. This way printf formatting
// can always use "%jd" or "%ju", and the compiler can do static analysis checks to verify the
// sign/unsigned nomenclature is matched.
//
// This is based on the same idea of how C compiler handles float/double internally already:
// that all float parameters are automatically promoted to double format when specified into
// va-args functions. I hope someday the C standard can somehow find a way to embrace the idea of
// doing the same for ints. If I want optimized codepaths, I'm not using va-args anyway. So let's
// just pick a size and make all parameters match it already. --jstine

static inline auto JFMT(const int8_t &  scalar) { return intmax_t(scalar); }
static inline auto JFMT(const int16_t&  scalar) { return intmax_t(scalar); }
static inline auto JFMT(const int32_t&  scalar) { return intmax_t(scalar); }
static inline auto JFMT(const int64_t&  scalar) { return intmax_t(scalar); }

static inline auto JFMT(const uint8_t & scalar) { return uintmax_t(scalar); }
static inline auto JFMT(const uint16_t& scalar) { return uintmax_t(scalar); }
static inline auto JFMT(const uint32_t& scalar) { return uintmax_t(scalar); }
static inline auto JFMT(const uint64_t& scalar) { return uintmax_t(scalar); }
