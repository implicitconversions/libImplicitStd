#pragma once

#include <array>

// ArrayEnumerable - a wrapper around std::array that allows accessing the array via an enum class.
// Normally the at() and operator[] require a cast to int. This wrapper implements type-strong overloads
// that allow avoiding that cast. 
template<class T, int SizeT, class EnumT> struct ArrayEnumerable : public std::array<T, SizeT> {
	using _parent = std::array<T, SizeT>;
	using _parent::operator[];
	using _parent::at;

	[[nodiscard]] auto& at(EnumT ch) {
		return ((_parent*)this)->at((int)ch);
	}

	[[nodiscard]] auto const& at(EnumT ch) const {
		return ((_parent*)this)->at((int)ch);
	}

	[[nodiscard]] auto& operator[](EnumT ch) {
		return ((_parent*)this)->at((int)ch);
	}

	[[nodiscard]] auto const& operator[](EnumT ch) const {
		return ((_parent*)this)->at((int)ch);
	}
};
