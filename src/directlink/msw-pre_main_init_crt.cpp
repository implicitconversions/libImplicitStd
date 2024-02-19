// Pre-Main Initialization Setup for Microsoft CRT And Windows Console
//
// Microsoft C/C++ Compiler/Linker:
//   This file must be manually included into EVERY PROJECT that intends to use the msw_app_console_init system.
//   Failure to add this file to the project which generates the executable will result in misbehavior of abort
//   dialogs, crash reporting, and console codepage selection. (does not apply to clang/gcc)
//
//
// [jstine 2022] I had developed a series of mechanations to try and force LINK to stuff this thing in the exe
//  manifest but none of them proved reliable. Whether or not the added CRT initializers show up in the EXE seems
//  to depend on some arbitrary order of linker inputs. It's a crapshoot. So after many tries, I've given up and
//  built this thing as a Loose TU to link directly into the C file. (linking obj file directly would also work
//  if we wanted to compile once and link to many places. But this seems OK for now and easier to visualize in
//  the Solution Explorer and less risk of dependency issues).

#if PLATFORM_MSW

#include "msw-app-console-init.h"
#include "LinkSectionComdat.h"

#include <corecrt_startup.h>

TU_DEBUGGABLE

static int _init_abort_behavior(void) {
	msw_InitAbortBehavior();
	return 0;
};

static int _init_console_behavior(void) {
	msw_SetConsoleCP();
	return 0;
};

#if _MSC_VER
// .CRT$XIC are the CRT C initializers, and anything run before those won't have stdio available.
//   This makes .CRT$XID a good spot for our own stuff to run as early as possible, and without crashing.

__section_declare_ro(".CRT$XIDA");
__section_declare_ro(".CRT$XIDB");

__section_item_ro(".CRT$XIDA")
_PIFV init_abort_behavior = _init_abort_behavior;

__section_item_ro(".CRT$XIDB")
_PIFV init_console_behavior = _init_console_behavior;

#else
// Assuming MSYS2 build environment on windows (clang/gcc)
struct A {
	A() {
		_init_abort_behavior();
	}
};

struct B {
	B() {
		_init_console_behavior();
	}
};

static A s_init_abort   __attribute__ ((init_priority (101)));
static B s_init_console __attribute__ ((init_priority (101)));

#endif


#endif
