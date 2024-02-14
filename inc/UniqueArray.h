#pragma once

#include <memory>

template<typename T>
class _UniqueArray_const_iterator
{

	// This is copied from STL std::array implementation, and a bunch of C++14/17/20 conditional boilerplate
	// removed in favor of targeting just what we need to care about. Also removes noexcept and constexpr
	// since the low-ROI clutter those inject into everything annoy me. --jstine

public:
	using pointer           = const T*;
	using reference         = const T&;

	_UniqueArray_const_iterator() : _Ptr() {}

	explicit _UniqueArray_const_iterator(pointer _Parg, size_t _Off = 0) : _Ptr(_Parg + _Off) {}

	[[nodiscard]] reference operator*() const {
		return *_Ptr;
	}

	[[nodiscard]] pointer operator->() const {
		return _Ptr;
	}

	_UniqueArray_const_iterator& operator++() {
		++_Ptr;
		return *this;
	}

	_UniqueArray_const_iterator operator++(int) {
		_UniqueArray_const_iterator _Tmp = *this;
		++_Ptr;
		return _Tmp;
	}

	_UniqueArray_const_iterator& operator--() {
		--_Ptr;
		return *this;
	}

	_UniqueArray_const_iterator operator--(int) {
		_UniqueArray_const_iterator _Tmp = *this;
		--_Ptr;
		return _Tmp;
	}

	_UniqueArray_const_iterator& operator+=(const ptrdiff_t _Off) {
		_Ptr += _Off;
		return *this;
	}

	[[nodiscard]] _UniqueArray_const_iterator operator+(const ptrdiff_t _Off) const {
		_UniqueArray_const_iterator _Tmp = *this;
		return _Tmp += _Off;
	}

	_UniqueArray_const_iterator& operator-=(const ptrdiff_t _Off) {
		_Ptr -= _Off;
		return *this;
	}

	[[nodiscard]] _UniqueArray_const_iterator operator-(const ptrdiff_t _Off) const {
		_UniqueArray_const_iterator _Tmp = *this;
		return _Tmp -= _Off;
	}

	[[nodiscard]] ptrdiff_t operator-(const _UniqueArray_const_iterator& _Right) const {
		return _Ptr - _Right._Ptr;
	}

	[[nodiscard]] reference operator[](const ptrdiff_t _Off) const {
		return _Ptr[_Off];
	}

	[[nodiscard]] bool operator==(const _UniqueArray_const_iterator& _Right) const {
		return _Ptr == _Right._Ptr;
	}

	// C++20 style, makes all the compatators obsolete.
	//[[nodiscard]] constexpr strong_ordering operator<=>(const _UniqueArray_const_iterator& _Right) const {
	//    return _Ptr <=> _Right._Ptr;
	//}

	[[nodiscard]] bool operator!=(const _UniqueArray_const_iterator& _Right) const {
		return !(*this == _Right);
	}

	[[nodiscard]] bool operator<(const _UniqueArray_const_iterator& _Right) const {
		return _Ptr < _Right._Ptr;
	}

	[[nodiscard]] bool operator>(const _UniqueArray_const_iterator& _Right) const {
		return _Right < *this;
	}

	[[nodiscard]] bool operator<=(const _UniqueArray_const_iterator& _Right) const {
		return !(_Right < *this);
	}

	[[nodiscard]] bool operator>=(const _UniqueArray_const_iterator& _Right) const {
		return !(*this < _Right);
	}

	using _Prevent_inheriting_unwrap = _UniqueArray_const_iterator;

	[[nodiscard]] constexpr pointer _Unwrapped() const {
		return _Ptr;
	}

	static constexpr bool _Unwrap_when_unverified = true;

	constexpr void _Seek_to(pointer _It) {
		_Ptr = _It;
	}

private:
	pointer _Ptr; // beginning of array
};

template<typename T>
class _UniqueArray_iterator : public _UniqueArray_const_iterator<T> {
public:
	using _Mybase = _UniqueArray_const_iterator<T>;

	using pointer           = T*;
	using reference         = T&;

	_UniqueArray_iterator() {}

	explicit _UniqueArray_iterator(pointer _Parg, size_t _Off = 0) : _Mybase(_Parg, _Off) {}

	[[nodiscard]] reference operator*() const {
		return const_cast<reference>(_Mybase::operator*());
	}

	[[nodiscard]] pointer operator->() const {
		return const_cast<pointer>(_Mybase::operator->());
	}

	_UniqueArray_iterator& operator++() {
		_Mybase::operator++();
		return *this;
	}

	_UniqueArray_iterator operator++(int) {
		_UniqueArray_iterator _Tmp = *this;
		_Mybase::operator++();
		return _Tmp;
	}

	_UniqueArray_iterator& operator--() {
		_Mybase::operator--();
		return *this;
	}

	_UniqueArray_iterator operator--(int) {
		_UniqueArray_iterator _Tmp = *this;
		_Mybase::operator--();
		return _Tmp;
	}

	_UniqueArray_iterator& operator+=(const ptrdiff_t _Off) {
		_Mybase::operator+=(_Off);
		return *this;
	}

	[[nodiscard]] _UniqueArray_iterator operator+(const ptrdiff_t _Off) const {
		_UniqueArray_iterator _Tmp = *this;
		return _Tmp += _Off;
	}

	_UniqueArray_iterator& operator-=(const ptrdiff_t _Off) {
		_Mybase::operator-=(_Off);
		return *this;
	}

	using _Mybase::operator-;

	[[nodiscard]] _UniqueArray_iterator operator-(const ptrdiff_t _Off) const {
		_UniqueArray_iterator _Tmp = *this;
		return _Tmp -= _Off;
	}

	[[nodiscard]] reference operator[](const ptrdiff_t _Off) const {
		return const_cast<reference>(_Mybase::operator[](_Off));
	}

	using _Prevent_inheriting_unwrap = _UniqueArray_iterator;

	[[nodiscard]] constexpr pointer _Unwrapped() const {
		return const_cast<pointer>(_Mybase::_Unwrapped());
	}
};

// A heap-allocated fixed-sized array of trivially default constructable objects. Uses unique_ptr
// internally to manage the array allocation. Can also think of this as a fixed-size array for
// situations where the size isn't known at compile time.
//
// Intended for use in place of std::vector, where the size is fixed and zero-fill initialization
// isn't desirable (which is usually always the case for any fixed pre-sized array situation).
template<typename T>
struct UniqueArray {
	std::unique_ptr<T[]> m_data;
	size_t m_size;

	UniqueArray(size_t size) : m_data(new T[size]) {
		static_assert(std::is_trivially_default_constructible<T>());
		m_size = size;
	}

	T* data() {
		return m_data.get();
	}

	T const* data() const {
		return m_data.get();
	}

	T* get() const {
		return m_data.get();
	}

	size_t size() const {
		return m_size;
	}

	size_t element_size() const {
		return sizeof(T);
	}

	T& operator[](int idx) {
		return m_data.get()[idx];
	}

	T const& operator[](int idx) const {
		return m_data.get()[idx];
	}

	// to allow compatiblity with template that accept std containers (aka vector)
	bool empty() const {
		return m_size == 0;
	}

	using iterator               = _UniqueArray_iterator<T>;
	using const_iterator         = _UniqueArray_const_iterator<T>;
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	[[nodiscard]] iterator begin() {
		return iterator(m_data.get(), 0);
	}

	[[nodiscard]] const_iterator begin() const {
		return const_iterator(m_data.get(), 0);
	}

	[[nodiscard]] iterator end() {
		return iterator(m_data.get(), m_size);
	}

	[[nodiscard]] const_iterator end() const {
		return const_iterator(m_data.get(), m_size);
	}

	[[nodiscard]] reverse_iterator rbegin() {
		return reverse_iterator(end());
	}

	[[nodiscard]] const_reverse_iterator rbegin() const {
		return const_reverse_iterator(end());
	}

	[[nodiscard]] reverse_iterator rend() {
		return reverse_iterator(begin());
	}

	[[nodiscard]] const_reverse_iterator rend() const {
		return const_reverse_iterator(begin());
	}

	[[nodiscard]] const_iterator cbegin() const {
		return begin();
	}

	[[nodiscard]] const_iterator cend() const {
		return end();
	}

	[[nodiscard]] const_reverse_iterator crbegin() const {
		return rbegin();
	}

	[[nodiscard]] const_reverse_iterator crend() const {
		return rend();
	}
};
