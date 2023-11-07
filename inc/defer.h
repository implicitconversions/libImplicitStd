#pragma once

#include <functional>

// -----------------------------------------------------------------------------------------------
// Defer (macro) / _impl_anon_defer_t (struct)

// Execute a function or lambda at the end of current lexical scope.
// This operates as an anonymous version of std::scope_exit with some additional function template assumptions to
// improve code gen:
//   - lambda lifetime is well-defined
//   - anonymous object cannot be referenced
//   - anonymous operation is not dismissable
//   - non-const reference storage used to disable accepting temporaries
//
template<typename T>
class _impl_anon_defer_t
{
private:
	T& m_defer;

public:
	explicit _impl_anon_defer_t(T& deferCb)
		: m_defer(deferCb)
	{
	}

	_impl_anon_defer_t(T&& fnDestruct) = delete;
	_impl_anon_defer_t(_impl_anon_defer_t const&) = delete;
	_impl_anon_defer_t(_impl_anon_defer_t&&) = delete;

	~_impl_anon_defer_t()
	{
		m_defer();
	}
};

#define _defer_expand_lineno_2(lineno, ...) \
	auto anon_defer_lambda_ ## lineno = [&]() { __VA_ARGS__; }; \
	auto anon_defer_        ## lineno = _impl_anon_defer_t { anon_defer_lambda_ ## lineno }

#define _deferFunc_expand_lineno_2(lineno, func) \
	auto anon_defer_        ## lineno = _impl_anon_defer_t { func }

#define _defer_expand_lineno_1(lineno, ...) _defer_expand_lineno_2(lineno, __VA_ARGS__)
#define _deferFunc_expand_lineno_1(lineno, func) _deferFunc_expand_lineno_2(lineno, func)


// defers a statement until the end of the current lexical scope. The parameter given must
// be a valid statement. Variables or pointers to functions are not permissible.
#define defer(...)   _defer_expand_lineno_1(__LINE__, __VA_ARGS__)

// defers a function invocation or lambda. The function signature must be no args, eg. func().
// intended use case is when an anonymous lambda defined by macro is too much of a pita to debug.
#define deferFunc(func) _deferFunc_expand_lineno_1(__LINE__, func)

#define Defer(...) defer(__VA_ARGS__)		// deprecated, prefer 'defer'
#define DeferFunc(func) deferFunc(func)		// deprecated, prefer 'deferFunc'

// helpful for showing intent when using defer as a lightweight local-scope pointer manager.
template<typename T>
T* ReleaseDefer(T* (&ptr)) {
	auto result = ptr;
	ptr = nullptr;
	return result;
}
