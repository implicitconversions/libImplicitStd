// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

#pragma once

#include <string>


#define EMBEDDED_BIN_TYPE_LD     1
#define EMBEDDED_BIN_TYPE_WIN32  2

#if PLATFORM_MSW

// Windows Resource (.rc) Style Embed
//
// Create foo.rc manually. Do not use the IDE! It makes an ugly .rc file with lots of mess.
// It's strongly recommended to give text and binary blobs their own .rc file to make it
// easier to create and modify these files directly (or with a makefile) rather than having
// to rely on the Visual Studio IDE.
//
// A simplified .rc file looks like this:

/*
#pragma code_page(65001)         // needed to prevent IDE from converting to UTF16
#include <winres.h>
//   identifier                 |  type    |   source path
// ---------------------------------------------------------------------------------------
   foo.lua                        RCDATA     "foo.txt"
   bar.bin                        RCDATA     "bar.bin"
*/

// identifier - a user-defined string used to fetch the resource. For file embedding this should
//   usually match the filename to make it easier to load the file either from embedded or from
//   filesystem if it exists there.
//
// type - should always be RCDATA (defined by Resource Compiler).
//
// source path - usually this is relative to project dir, which si the dir msbuild launches
//   Resource Compiler from.

extern std::tuple<void const*,intmax_t> msw_GetResourceInfo(std::string resource_filename);
#endif

// Linker Object (ld) Style Embed
//
// This style of embed is easy to use from Clang/gcc but could also be applied to Microsoft
// C++ toolchain in various ways, such as using a bin2h step followed by cl.exe to generate
// the object file). Therefore the macros and code for this path are made available without
// compiler or platform conditionals.
//
// Import textfile "foo.txt" using the following:
//
//    ld -r -b binary -o foo.txt.o foo.txt
//
// That object file provides the following symbols:
//
//      _binary_foo_txt_start
//      _binary_foo_txt_end
//      _binary_foo_txt_size
//
// All symbols are addresses, including size. To get the size value, take the address
// of the _binary_foo_txt_size symbol.

// this macro can be used to import a binary which has been included by way of:
//     ld -r -b binary -o foo.txt.o foo.txt
#	define EmbeddedBinaryDataImport(name) \
		extern "C" char _binary_##name##_start[], \
		                _binary_##name##_end[], \
					    _binary_##name##_size[]


