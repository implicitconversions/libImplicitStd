#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////
// pragma_todo (macro)
// Used to produce handy-dandy messages like:
//    1> C:\Source\Project\main.cpp(47): Reminder: Fix this problem!

#define _pragmahelper_Stringize( L )       #L
#define _pragmahelper_MakeString( M, L )   M(L)
#define _pragmahelper_Line                 _pragmahelper_MakeString( _pragmahelper_Stringize, __LINE__ )
#define _pragmahelper_location             __FILE__ "(" _pragmahelper_Line "): "

#if !defined(SHOW_PRAGMA_TODO)
#   define SHOW_PRAGMA_TODO         1
#endif

#if SHOW_PRAGMA_TODO
#   if defined(_MSC_VER)
#       define pragma_todo(...)     __pragma    (message        (_pragmahelper_location "-TODO- " __VA_ARGS__))
#   else
		// clang pragma messages are ugly. Turn them off for now...
#       define pragma_todo(...)
//#       define pragma_todo(...)     _Pragma     (_pragmahelper_Stringize(GCC warning(_pragmahelper_location "-TODO- " __VA_ARGS__)))
#   endif
#else
#   define pragma_todo(...)
#endif
