// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

// x64 architectures are 64-byte cacheline.
#if !defined(ATOMIC_CACHELINE_SIZE)
#	define ATOMIC_CACHELINE_SIZE  64
#endif

// alignas_atomic / __aligned_atomic
// For best performance, 32 and 64-bit values used as inputs to Atomic API functions should be
// aligned to a cacheline boundary (64-bytes on most architectures).  Having a mixture of reads
// and writes to different parts of a given 64-byte cacheline can result in inter-core and especially
// inter-cluster cache bus hazards (potentially in the neighborhood of ~1500 cycles on AMD Jaguar).
//
// Exceptions to the Rule:  it's ok if an atomic variable shares a cacheline with non-atomic data.
// If there are mutliple atomic vars in a struct then they need to each have their own cacheline.
#if defined(_MSC_VER)
#	define __aligned_atomic		__declspec(align(ATOMIC_CACHELINE_SIZE))
#else
#	define __aligned_atomic		__attribute__((aligned(ATOMIC_CACHELINE_SIZE)))
#endif

// c++ naming convention for __aligned_atomic
#define alignas_atomic   __aligned_atomic


////////////////////////////////////////////////////////////////////////////////////////
// Atomics.
//
// Rationale:
//   Yes C++11 has its own now. But we are not interested in using them because we don't need to care
//   about memory order things.  For one, they're ineffective on AMD64 systems, which implicitly impose
//   __ATOMIC_SEQ_CST on all atomic operations except AtomicLoads. Furthermore, the actual use cases
//   for anything other than either __ATOMIC_SEQ_CST or __ATOMIC_RELEASE are slim to none even on
//   platforms where it matters:
//
//     * by doing __ATOMIC_RELEASE on the thread that writes data you don't need __ATOMIC_ACQUIRE on any
//       consumer thread (and its really really hard to write code that uses __ATOMIC_ACQUIRE instead of
//       release and have it be properly coherent and for the wee tiny perf boost who cares?
//
//     * __ATOMIC_CONSUME is similar to using the volatile keyword, as it only affects compiler optimiz-
//       ations and instruction ordering. It's more aggressive than volatile as it affects all variables
//       that might be used to generate the value that is being written via atomic consume. See notes
//       above about cachelines and stuff on why this is probably a tragic and terrible idea for perf.
//       If we need to be communicating entire structs of data we need to do it in a designed fashion
//       that matches our target platform, or we do it lazily using mutex. __ATOMIC_CONSUME therefore is
//       useless.
//
//     * __ATOMIC_ACQ_REL is some kind of unknown. It's not well explained, and appears to be some hybrid
//       mix of compiler behavior and underlying hardware behavior. From reading my hunch is it's some-
//       thing that is only applicable to SPARC and Itanium systems. To be sure it's useless on AMD64 or
//       ARM64.
//
//   Final reason is performance in debug builds. For the same reason we've explicitly copied and pasted
//   some things below which could have called other implementations. Debug builds lack inlining and we
//   want to avoid a whole bunch of nested calls into std:memcpy.
//


template< typename T > yesinline T volatile& volatize(volatile T& src) {
	return reinterpret_cast<T volatile&>(src);
}

template< typename T > yesinline volatile T volatize(volatile T const& src) {
	return reinterpret_cast<volatile T const&>(src);
}

// atomic loads on x64 architectures are fine to just rely on compiler barriers. Memory fences on loads
// are not needed and generally not recommended. Fencing and order of writes should be enforced by the
// store/write threads, which tends lead to improved CPU cache performance broadly.
template< typename T > yesinline T AtomicLoad(volatile T const&  src)  {
	return reinterpret_cast<volatile T const&>(src);
}


#ifdef _MSC_VER

#include <intrin.h>

// Note that Microsoft's implementation of Interlocked functions predate stdint, so they have
// typically wonky varieties of int, long, and long long... ugh. --jstine
using _interlocked_s32 = long volatile*;
using _interlocked_s8  = char volatile*;

yesinline static int8_t  AtomicExchange			(volatile int8_t& dest, int8_t src )								{ return _InterlockedExchange8			((_interlocked_s8 ) &dest, src); }
yesinline static int8_t  AtomicExchangeAdd		(volatile int8_t& src,  int8_t amount )								{ return _InterlockedExchangeAdd8		((_interlocked_s8 ) &src, (long)amount); }
yesinline static int8_t  AtomicCompareExchange	(volatile int8_t& dest, int8_t exchange, int8_t comparand )			{ return _InterlockedCompareExchange8	((_interlocked_s8 ) &dest, exchange, comparand); }

static_assert(sizeof(bool) == 1);
yesinline static bool    AtomicExchange			(volatile bool& dest, bool src )									{ return _InterlockedExchange8			((_interlocked_s8 ) &dest, src); }
yesinline static bool    AtomicCompareExchange	(volatile bool& dest, bool exchange, bool comparand )				{ return _InterlockedCompareExchange8	((_interlocked_s8 ) &dest, exchange, comparand); }

yesinline static int32_t  AtomicInc				(volatile int32_t& val)												{ return _InterlockedIncrement			((_interlocked_s32 ) &val); }
yesinline static int64_t  AtomicInc				(volatile int64_t& val)												{ return _InterlockedIncrement64		((int64_t volatile*) &val); }
yesinline static int32_t  AtomicDec				(volatile int32_t& val)												{ return _InterlockedDecrement			((_interlocked_s32 ) &val); }
yesinline static int64_t  AtomicDec				(volatile int64_t& val)												{ return _InterlockedDecrement64		((int64_t volatile*) &val); }
yesinline static int32_t  AtomicExchange			(volatile int32_t& dest, int32_t src )								{ return _InterlockedExchange			((_interlocked_s32 ) &dest, src); }
yesinline static int64_t  AtomicExchange			(volatile int64_t& dest, int64_t src )								{ return _InterlockedExchange64			((int64_t volatile*) &dest, src); }
yesinline static int32_t  AtomicExchangeAdd		(volatile int32_t& src,  int32_t amount )							{ return _InterlockedExchangeAdd		((_interlocked_s32 ) &src, (long)amount); }
yesinline static int64_t  AtomicExchangeAdd		(volatile int64_t& src,  int64_t amount )							{ return _InterlockedExchangeAdd64		((int64_t volatile*) &src, amount); }
yesinline static int32_t  AtomicCompareExchange	(volatile int32_t& dest, int32_t exchange, int32_t comparand )		{ return _InterlockedCompareExchange	((_interlocked_s32 ) &dest, exchange, comparand); }
yesinline static int64_t  AtomicCompareExchange	(volatile int64_t& dest, int64_t exchange, int64_t comparand )		{ return _InterlockedCompareExchange64	((int64_t volatile*) &dest, exchange, comparand); }

yesinline static uint32_t AtomicInc				(volatile uint32_t& val)											{ return _InterlockedIncrement			((_interlocked_s32 ) &val); }
yesinline static uint64_t AtomicInc				(volatile uint64_t& val)											{ return _InterlockedIncrement64		((int64_t volatile*) &val); }
yesinline static uint32_t AtomicDec				(volatile uint32_t& val)											{ return _InterlockedDecrement			((_interlocked_s32 ) &val); }
yesinline static uint64_t AtomicDec				(volatile uint64_t& val)											{ return _InterlockedDecrement64		((int64_t volatile*) &val); }
yesinline static uint32_t AtomicExchange			(volatile uint32_t& dest, uint32_t src )							{ return _InterlockedExchange			((_interlocked_s32 ) &dest, src); }
yesinline static uint64_t AtomicExchange			(volatile uint64_t& dest, uint64_t src )							{ return _InterlockedExchange64			((int64_t volatile*) &dest, src); }
yesinline static uint32_t AtomicExchangeAdd		(volatile uint32_t& src,  uint32_t amount )							{ return _InterlockedExchangeAdd		((_interlocked_s32 ) &src, (long)amount); }
yesinline static uint64_t AtomicExchangeAdd		(volatile uint64_t& src,  uint64_t amount )							{ return _InterlockedExchangeAdd64		((int64_t volatile*) &src, amount); }
yesinline static uint32_t AtomicCompareExchange	(volatile uint32_t& dest, uint32_t exchange, uint32_t comparand )	{ return _InterlockedCompareExchange	((_interlocked_s32 ) &dest, exchange, comparand); }
yesinline static uint64_t AtomicCompareExchange	(volatile uint64_t& dest, uint64_t exchange, uint64_t comparand )	{ return _InterlockedCompareExchange64	((int64_t volatile*) &dest, exchange, comparand); }

template<class T> yesinline static T* AtomicExchangePointer	(T* (&dest), T* src)									{ return static_cast<T*>(_InterlockedExchangePointer((void* volatile*) &dest, src)); }
template<class T> yesinline static T* AtomicExchangePointer	(T* (&dest), std::nullptr_t src)						{ return static_cast<T*>(_InterlockedExchangePointer((void* volatile*) &dest, src)); }

template<class T> yesinline static T* AtomicCompareExchangePointer(T* (&dest), T* exchange, T* comparand)			    { return static_cast<T*>(_InterlockedCompareExchangePointer((void* volatile*) &dest, exchange, comparand)); }
template<class T> yesinline static T* AtomicCompareExchangePointer(T* (&dest), T* exchange, std::nullptr_t comparand) { return static_cast<T*>(_InterlockedCompareExchangePointer((void* volatile*) &dest, exchange, comparand)); }

template< typename T >
yesinline T AtomicExchangeEnum( volatile T& dest, T src )
{
	// for atomic exchange purposes, we can disregard sign. Only size matters.
	if constexpr (sizeof(T) == 1) {
		return (T)_InterlockedExchange8  ((_interlocked_s8  ) &dest, (int8_t)src);
	}
	elif constexpr (sizeof(T) == 4) {
		return (T)_InterlockedExchange   ((_interlocked_s32 ) &dest, (int32_t)src);
	}
	elif constexpr (sizeof(T) == 8) {
		return (T)_InterlockedExchange64 ((int64_t volatile*) &dest, (int64_t)src);
	}
	else {
		return "error: unsupported enum size type";		// will surface as a compiler return type mismatch
	}
}

template< typename T >
yesinline T AtomicCompareExchangeEnum( volatile T& dest, T exchange, T comparand )
{
	// for atomic exchange purposes, we can disregard sign. Only size matters.
	if constexpr (sizeof(T) == 1) {
		return (T)_InterlockedCompareExchange8 ((_interlocked_s8   ) &dest, (int8_t)exchange, (int8_t)comparand);
	}
	elif constexpr (sizeof(T) == 4) {
		return (T)_InterlockedCompareExchange  ((_interlocked_s32  ) &dest, (int32_t)exchange, (int32_t)comparand);
	}
	elif constexpr (sizeof(T) == 8) {
		return (T)_InterlockedCompareExchange64	((int64_t volatile*) &dest, (int64_t)exchange, (int64_t)comparand);
	}
	else {
		return "error: unsupported enum size type";		// will surface as a compiler return type mismatch
	}
}

#else

// GCC/Clang support identical implementation
//
// And more remarks, because the internet is full of people who know very little about actual
// async threading programming and memory ordering:
//
// * weak vs strong atomic compare exchange: the purpose there is a micro-optimization, mostly just for ARM.
//   On some platforms (ARM!) a compare-exchange must always be implemented as a loop of load/store atomics,
//   because there's no actual atomic operation for it. On just about everything else loops need only be used
//   when it's important to ensure two threads don't overwrite each other's modifications to a variable.
//   In the latter case, the generated code would be two loops -- one to simulate exchange, and one to
//   verify the result. So for those platforms a 'weak' version is better, it compiles to just the outer
//   loop! But the real issue is that trying to architect exchnage-based thread constructs on platforms
//   that don't actually support it is a BAD IDEA. If you actually care about perf on that platform, re-
//   architect your async threads so they don't rely on so many exchanges.  --jstine


#define _tso_atomic_policy __ATOMIC_SEQ_CST

yesinline static int8_t  AtomicExchange			(volatile int8_t& dest, int8_t src )								{ return __atomic_exchange_n	(&dest, src, _tso_atomic_policy); }
yesinline static int8_t  AtomicExchangeAdd		(volatile int8_t& src,  int8_t amount )								{ return __atomic_fetch_add		(&src, amount, _tso_atomic_policy); }
yesinline static int8_t  AtomicCompareExchange	(volatile int8_t& dest, int8_t exchange, int8_t comparand )			{ __atomic_compare_exchange_n	(&dest, &comparand, exchange, 0, _tso_atomic_policy, _tso_atomic_policy); return comparand; }

static_assert(sizeof(bool) == 1);
yesinline static int8_t  AtomicExchange			(volatile bool& dest, bool src )									{ return __atomic_exchange_n	(&dest, src, _tso_atomic_policy); }
yesinline static int8_t  AtomicCompareExchange	(volatile bool& dest, bool exchange, bool comparand )				{ __atomic_compare_exchange_n	(&dest, &comparand, exchange, 0, _tso_atomic_policy, _tso_atomic_policy); return comparand; }

yesinline static int32_t  AtomicInc				(volatile int32_t& val)												{ return __atomic_fetch_add		(&val,  1, _tso_atomic_policy) + 1; }
yesinline static int64_t  AtomicInc				(volatile int64_t& val)												{ return __atomic_fetch_add		(&val,  1, _tso_atomic_policy) + 1; }
yesinline static int32_t  AtomicDec				(volatile int32_t& val)												{ return __atomic_fetch_add		(&val, -1, _tso_atomic_policy) - 1; }
yesinline static int64_t  AtomicDec				(volatile int64_t& val)												{ return __atomic_fetch_add		(&val, -1, _tso_atomic_policy) - 1; }
yesinline static int32_t  AtomicExchange			(volatile int32_t& dest, int32_t src )								{ return __atomic_exchange_n	(&dest, src, _tso_atomic_policy); }
yesinline static int64_t  AtomicExchange			(volatile int64_t& dest, int64_t src )								{ return __atomic_exchange_n	(&dest, src, _tso_atomic_policy); }
yesinline static int32_t  AtomicExchangeAdd		(volatile int32_t& src,  int32_t amount )							{ return __atomic_fetch_add		(&src, amount, _tso_atomic_policy); }
yesinline static int64_t  AtomicExchangeAdd		(volatile int64_t& src,  int64_t amount )							{ return __atomic_fetch_add		(&src, amount, _tso_atomic_policy); }
yesinline static int32_t  AtomicCompareExchange	(volatile int32_t& dest, int32_t exchange, int32_t comparand )		{ __atomic_compare_exchange_n	(&dest, &comparand, exchange, 0, _tso_atomic_policy, _tso_atomic_policy); return comparand; }
yesinline static int64_t  AtomicCompareExchange	(volatile int64_t& dest, int64_t exchange, int64_t comparand )		{ __atomic_compare_exchange_n	(&dest, &comparand, exchange, 0, _tso_atomic_policy, _tso_atomic_policy); return comparand; }

yesinline static uint32_t AtomicInc				(volatile uint32_t& val)											{ return __atomic_fetch_add		(&val,  1, _tso_atomic_policy) + 1; }
yesinline static uint64_t AtomicInc				(volatile uint64_t& val)											{ return __atomic_fetch_add		(&val,  1, _tso_atomic_policy) + 1; }
yesinline static uint32_t AtomicDec				(volatile uint32_t& val)											{ return __atomic_fetch_add		(&val, -1, _tso_atomic_policy) - 1; }
yesinline static uint64_t AtomicDec				(volatile uint64_t& val)											{ return __atomic_fetch_add		(&val, -1, _tso_atomic_policy) - 1; }
yesinline static uint32_t AtomicExchange			(volatile uint32_t& dest, uint32_t src )							{ return __atomic_exchange_n	(&dest, src, _tso_atomic_policy); }
yesinline static uint64_t AtomicExchange			(volatile uint64_t& dest, uint64_t src )							{ return __atomic_exchange_n	(&dest, src, _tso_atomic_policy); }
yesinline static uint32_t AtomicExchangeAdd		(volatile uint32_t& src,  uint32_t amount )							{ return __atomic_fetch_add		(&src, amount, _tso_atomic_policy); }
yesinline static uint64_t AtomicExchangeAdd		(volatile uint64_t& src,  uint64_t amount )							{ return __atomic_fetch_add		(&src, amount, _tso_atomic_policy); }
yesinline static uint32_t AtomicCompareExchange	(volatile uint32_t& dest, uint32_t exchange, uint32_t comparand )	{ __atomic_compare_exchange_n(&dest, &comparand, exchange, 0, _tso_atomic_policy, _tso_atomic_policy); return comparand; }
yesinline static uint64_t AtomicCompareExchange	(volatile uint64_t& dest, uint64_t exchange, uint64_t comparand )	{ __atomic_compare_exchange_n(&dest, &comparand, exchange, 0, _tso_atomic_policy, _tso_atomic_policy); return comparand; }

template<class T> yesinline static T* AtomicExchangePointer	(T* (&dest), T* src)									{ return __atomic_exchange_n	(&dest, src, _tso_atomic_policy); }
template<class T> yesinline static T* AtomicExchangePointer	(T* (&dest), std::nullptr_t src)						{ return __atomic_exchange_n	(&dest, src, _tso_atomic_policy); }

template<class T> yesinline static T* AtomicCompareExchangePointer(T* (&dest), T* exchange, T* comparand)			    { __atomic_compare_exchange_n(&dest,      &comparand, exchange, 0, _tso_atomic_policy, _tso_atomic_policy); return comparand; }
template<class T> yesinline static T* AtomicCompareExchangePointer(T* (&dest), T* exchange, std::nullptr_t comparand) { __atomic_compare_exchange_n(&dest, (T**)&comparand, exchange, 0, _tso_atomic_policy, _tso_atomic_policy); return comparand; }

template< typename T >
yesinline T AtomicExchangeEnum( volatile T& dest, T src)
{
	return __atomic_exchange_n(&dest, src, _tso_atomic_policy);
}

template< typename T >
yesinline T AtomicCompareExchangeEnum( volatile T& dest, T exchange, T comparand )
{
	__atomic_compare_exchange_n(&dest, &exchange, comparand, 0, _tso_atomic_policy, _tso_atomic_policy);
	return exchange;
}

#endif
