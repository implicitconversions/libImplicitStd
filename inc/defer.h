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

#define _defer_expand_counter_2(func, count) \
	auto anon_defer_lambda_ ## count = [&]() { func; }; \
	auto anon_defer_        ## count = _impl_anon_defer_t { anon_defer_lambda_ ## count }

#define _defer_expand_counter_1(func, count) _defer_expand_counter_2(func, count)

// defers a statement until the end of the current lexical scope. The parameter given must
// be a valid statement. Variables or pointers to functions are not permissible.
#define Defer(function_content)   _defer_expand_counter_1(function_content, __COUNTER__ )
