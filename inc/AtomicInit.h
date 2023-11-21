#pragma once

#include "atomics.h"
#include "defer.h"

struct AtomicInitRecursionGuardStackItem
{
	int8_t* pInitStat;
	struct AtomicInitRecursionGuardStackItem* next;
};

static constexpr int8_t kAtomicOnceDoneBit = (1ul << 7);

extern int32_t atomicInitIsFirst(int8_t* init, AtomicInitRecursionGuardStackItem* guard);
extern void    atomicInitSetDone(int8_t* init, AtomicInitRecursionGuardStackItem* guard);

#define _internal_ATOMIC_INIT_GUARD_2(initvar, guardvar, lineno)    \
	static int8_t initvar##lineno;									\
	AtomicInitRecursionGuardStackItem guardvar##lineno { &initvar##lineno };	\
	if(expect_true( (initvar##lineno & kAtomicOnceDoneBit)!=0 ))    \
		return;														\
	if (!atomicInitIsFirst(&initvar##lineno, &guardvar##lineno))	\
		return;														\
	defer(atomicInitSetDone(&initvar##lineno, &guardvar##lineno))

#define _internal_ATOMIC_INIT_EXPR_2(initvar, guardvar, lineno, expr) [&]{ \
	static int8_t initvar##lineno;									\
	AtomicInitRecursionGuardStackItem guardvar##lineno { &initvar##lineno };	\
	if(expect_false( (initvar##lineno & kAtomicOnceDoneBit)==0 )	\
		&& atomicInitIsFirst(&initvar##lineno, &guardvar##lineno))	\
	{																\
		defer(atomicInitSetDone(&initvar##lineno, &guardvar##lineno));	\
		expr;														\
	} \
}();


struct _AtomicInitGuardImpl {
	int8_t& static_initvar;
	AtomicInitRecursionGuardStackItem guardvar;
	bool needsSetDone = false;

	_AtomicInitGuardImpl(int8_t& initvar) :
		static_initvar(initvar), guardvar{&initvar}
	{
	}

	~_AtomicInitGuardImpl() {
		if (needsSetDone) {
			atomicInitSetDone(&static_initvar, &guardvar);
			needsSetDone = false;
		}
	}

	bool _bool_conversion() {
		if (expect_true( (static_initvar & kAtomicOnceDoneBit)!=0 )) {
			return true;
		}
		if (!atomicInitIsFirst(&static_initvar, &guardvar)) {
			return true;
		}

		needsSetDone = true;
		return false;
	}

	operator bool() { return _bool_conversion(); }
};

#define _internal_ATOMIC_INIT_EXPR(initvar, guardvar, lineno, expr) _internal_ATOMIC_INIT_EXPR_2(initvar, guardvar, lineno, expr)
#define _internal_ATOMIC_INIT_GUARD(initvar, guardvar, lineno)      _internal_ATOMIC_INIT_GUARD_2(initvar, guardvar, lineno)


#define ATOMIC_INIT_EXPR(expr) _internal_ATOMIC_INIT_EXPR(s_atomicInit_, atomicRecursionGuard_, __LINE__, expr)

// Deprecated: Use if(ATOMIC_INIT_IS_DONE()) return; instead.
// usage:
//   void foo() {
//       ATOMIC_INIT_GUARD;
//       [statements]
//   }
#define ATOMIC_INIT_GUARD      _internal_ATOMIC_INIT_GUARD(s_atomicInit_, atomicRecursionGuard_, __LINE__)

// First to reach this condition runs, other threads block until the first is completed.
// Usage:
//   StaticVarType foo() {
//       static StaticVarType s_staticvar;
//       if (ATOMIC_INIT_IS_FIRST()) {
//           [init statements]
//       }
//       return s_staticvar;
//   }
#define ATOMIC_INIT_IS_FIRST()  expect_false([]() { static int8_t initvar; return !_AtomicInitGuardImpl(initvar)._bool_conversion(); }())

// Usage:
//   void foo() {
//       if (ATOMIC_INIT_IS_DONE()) {
//           return;
//       }
//       [init statements]
//   }
#define ATOMIC_INIT_IS_DONE()   expect_true(!ATOMIC_INIT_IS_FIRST())
