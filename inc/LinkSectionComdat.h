#pragma once

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

// Initializer Section Names explained:
// .CRT$XIC are the CRT C initializers, and anything run before those won't have stdio available.
// .CRT$XID is a good spot for stuff to run as early as possible and without also crashing.

#if _MSC_VER
#	define __section_declare_ro(section_name) __pragma (section  (section_name, read))
#	define __section_item_ro(section_name)    __declspec(allocate(section_name))
#else
#	define PRAGMA_SECTION(arg) __pragma(gcc section rodata=arg)

#	define __section_declare_ro(section_name)
#	define __section_item_ro(section_name)    PRAGMA_SECTION(section_name)
#endif
