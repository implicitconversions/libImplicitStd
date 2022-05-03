#pragma once

#define ICY_EVAL(s) #s

#define _ICY_CONCAT_(l,r) l##r
#define ICY_CONCAT(l,r) _ICY_CONCAT_(l,r)

#define _IMPL_AUTO_ANON(line) auto ICY_CONCAT(_auto_anonymous_, line) =
#define AUTO_ANON _IMPL_AUTO_ANON(__LINE__)
