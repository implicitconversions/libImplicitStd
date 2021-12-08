#pragma once

#include <functional>

// -----------------------------------------------------------------------------------------------
// Defer (macro) / Defer_t (struct)
//
// Inspired by Golang's 'defer' keyword.  Allows binding a lambda to be executed when the current
// scope of the deferral's creation is left.  This still differs from Golang `defer`:
//
//    - Golang defer executes at the end of function scope.
//    - Our C++ Defer executes at the end of current lexical scope.
//
// Advantages:
//   and also allows defining a custom deletion/destruction action without having to create
//   custom wrapper class.  If you want a long-winded version, then use Defer_t directly.
struct Defer_t {
	std::function<void()>   m_func;

private:
	Defer_t(const Defer_t& rvalue)         = delete;
	void operator=(const Defer_t& rvalue)  = delete;

public:
	Defer_t(Defer_t&& rvalue) {
		m_func = std::move(rvalue.m_func);
		rvalue.m_func = nullptr;
	}

	Defer_t() throw() { }
	Defer_t(const std::function<void()>& func) throw() {
		m_func = func;
	}

	~Defer_t() {
		m_func();
	}

	void Bind(const std::function<void()>& func) {
		m_func = func;
	}
};

// -----------------------------------------------------------------------------------------------
// Defer Macros
//
// DeferL  - Accepts a lambda or a function pointer as the parameter.
//
// Defer - This has the lambda syntax and semi-colon baked-in on purpose.  The point of this macro is maximum
//   brevity for the most common case usage, which is to free a pointer or release a handle.

#define _defer_expand_counter_2(func,count) Defer_t defer_anon_ ## count( func )
#define _defer_expand_counter_1(func,count) _defer_expand_counter_2(func, count)
#define DeferL(func) _defer_expand_counter_1(func, __COUNTER__)
#define Defer(function_content) DeferL( [&]() { function_content; } )
