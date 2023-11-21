#include "AtomicInit.h"
#include "icy_assert.h"

#include <chrono>
#include <thread>

static thread_local AtomicInitRecursionGuardStackItem* s_atomicInitRecursionGuardStack;

// intended for use from a macro wrapper that shortcuts past this call when kAtomicOnceDoneBit is already set.
__noinline int atomicInitIsFirst(int8_t* init, AtomicInitRecursionGuardStackItem* guard)
{
	// First read the value (atomically), because any value besides 0 lets us skip trying to write.
	// Some archs have hazards on lots of spurious interlocked memory (either at CPU cache or bus levels), and
	// since this is a slowpath it's fine to do a little extra local work to avoid polluting the memory bus.

	// This atomic_load isn't required: the atomic exchange at the end of the block generates all the necessary
	// compiler barriers. But since we're a slowpath behind the kAtomicOnceDoneBit gated check, there's no harm
	// in pedantically checking...

	if (AtomicLoad(*init) == 0) {
		if (AtomicCompareExchange(*init, 1, 0) == 0) {
			// Of all the threads that try the CAS, only the first one gets here.
			if (guard) {
				guard->next = s_atomicInitRecursionGuardStack;
				guard->pInitStat = init;
				s_atomicInitRecursionGuardStack = guard;
			}
			return 1;
		}
	}

	if (guard) {
		auto* next = s_atomicInitRecursionGuardStack;

		while (next)	{
			if (expect_false(next->pInitStat == init)) {
				icyReportError("Recursion detected at ATOMIC_INIT_BEGIN.");
				return 0;
			}
		}
	}

	// "spin" until the first thread calls atomicInitSetDone
	// TODO: this could be replaced with a single global condvar/sema. The sema might wake up multiple times until
	// the right thread finishes (if multiple different inits are in progress) but that's still better than sleep(1).
	// Care should be taken to ensure satisfactory behavior if the sema is not yet init'd.

	while (!(AtomicLoad(*init) & kAtomicOnceDoneBit))
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2000ms);
	}

	return 0;
}

// intended for use from a macro wrapper that only invokes this call when atomicInitIsFirst returns TRUE.
void atomicInitSetDone(int8_t* init, AtomicInitRecursionGuardStackItem* guard)
{
	// since recursion is disallowed, and the guard is a thread-local stack, the most recent item on the guard
	// *must* be ours.
	if (guard)
	{
		assertD(s_atomicInitRecursionGuardStack == guard && s_atomicInitRecursionGuardStack->pInitStat == init);
		s_atomicInitRecursionGuardStack = s_atomicInitRecursionGuardStack->next;
	}

	AtomicExchange(*init, kAtomicOnceDoneBit);
}
