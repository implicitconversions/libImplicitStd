#pragma once

#include <atomic>

#include "MacroUtil.h"

struct ForceSymbolToBeLinked
{
	ForceSymbolToBeLinked() = delete;

	template<typename T>
	ForceSymbolToBeLinked(T&& p) {
		_ForceSymbolToBeLinked((void*)&p);
	}

	void _ForceSymbolToBeLinked(void const* p);
};

#define _FORCE_LINK_SYMBOL_LINE(p) ForceSymbolToBeLinked ICY_CONCAT(_deadref_, __LINE__) (p)

#define ForceLinkSymbol(p)  _FORCE_LINK_SYMBOL_LINE(p)
#define ForceLinkLiteral(p) _FORCE_LINK_SYMBOL_LINE("" p)


// Section Specification.
// Microsoft has a remarkably obtuse way of defining sections compared to CLANG.
//  - Microsoft requires a section to be declared and then each line item must be assigned to that section
//    using __declspec repeatedly.
//  - Microsoft doesn't allow specifying a bss type section.
//  - Clang allows a section to be defined once, and then all items in the rest of TU are put into that
//    section.
//  - Clang DOES allow specifying a bss type section.
//
// Because of this, I strongly recommend only using this feature for very trivial RW, RO, and Exec sections.
// Be aware also that it's possible to very accidentally put line items into entirely different sections
// on each compiler by way of lazy typo. I strongly recommend using additional macro wrappers for any non-
// trivial use cases to help avoid that annoyance.

#if _MSC_VER
#	define __section_declare_ro(section_name) __pragma (section  (section_name, read))
#	define __section_item_ro(section_name)    __declspec(allocate(section_name))
#elif defined(__clang__)
#	define PRAGMA_SECTION(arg) __pragma(clang section rodata=arg)

#	define __section_declare_ro(section_name)
#	define __section_item_ro(section_name)    PRAGMA_SECTION(section_name)
#else
#	error Unimplemented compiler macro.
#endif
