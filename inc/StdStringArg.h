#pragma once

#include <string>
#include <string_view>
#include <cstring>
#include <climits>
#include <cassert>

// StringConversionMagick - struct meant for use as an aide in function parameter passing only.
//
// Rationale:
//   C++ has a rule that we can't use references as the fmt parameter to a varadic function,
//   which prevents us from making a nice API that can accept std::string or const char* implicitly.
//   In the past I've worked around this by making my own wrapper class for std::string that has
//   an implicit conversion operator for const char*(), but that's a pretty heavyweight approach that
//   makes it really hard for libraries to inter-operate with other libs that expect plain old
//   std::string.  So now I'm going with this, let's see what happens!  --jstine
//
struct StringConversionMagick {
	const char*			m_cstr   = nullptr;
	const std::string*	m_stdstr = nullptr;
	int                 m_length = -1;

	StringConversionMagick(const std::string& str) {
		m_stdstr = &str;
	}

	StringConversionMagick(const char* const (&str)) {
		m_cstr   = str;
	}

	template<int size>
	StringConversionMagick(const char (&str)[size]) {
		m_cstr   = str;
		m_length = size-1;
	}

	bool isStdString() const {
		return m_stdstr;
	}

	std::string const* string_ptr() const {
		return m_stdstr;
	}

	const char* c_str() const {
		return m_stdstr ? m_stdstr->c_str() : m_cstr;
	}

	bool empty() const {
		if (m_stdstr) {
			return m_stdstr->empty();
		}
		return !m_cstr || !m_cstr[0];
	}

	auto length() const {
		if (m_stdstr) return m_stdstr->length();
		return (m_length < 0) ? strlen(m_cstr) : m_length;
	}
};


using StdStringTempArg = StringConversionMagick const&;

// StringViewSpecificArg - struct meant for use as an aide in function parameter passing only.
// Allows passing char*, string, or string_view into a function. The function is then to use the 
// read_next/peek_next methods to read out the string. Suitable for use by functions that would
// normally be well-suited to ASCII-Z strings, eg. where the length of the string does not need
// to be known ahead of time.
// 
// Templating is employed to maximize codegen. Use StringViewGenericArg for un-optimized general solution.
template<bool allowCString=true, bool allowStringView=true>
struct StringViewSpecificArg {
	char const*				m_cstr   = nullptr;
	std::string_view		m_stdstr;
	int                     m_readpos = 0;

	StringViewSpecificArg(std::string const& str) : m_stdstr(str) {
	}

	StringViewSpecificArg(std::string_view const& str) : m_stdstr(str) {
	}

	StringViewSpecificArg(char const* const (&str)) {
		m_cstr   = str;
	}

	template<int size>
	StringViewSpecificArg(const char (&str)[size]) : m_stdstr(str, size) {
	}

	yesinline
	bool isView() const {
		if constexpr (allowCString) {
			return !m_cstr;
		}
		else {
			return true;
		}
	}

	yesinline
	int read_next() {
		int result = peek_next();
		m_readpos += (result != 0);
		return result;
	}

	yesinline
	int peek_next() const {
		if constexpr (allowCString) {
			if (!isView()) {
				auto result = m_cstr[m_readpos];
				return result;
			}
		}

		if constexpr(allowStringView) {
			if (m_readpos < m_stdstr.length()) {
				return m_stdstr[m_readpos];
			}
		}
		return 0;
	}

	yesinline
	int readpos() const {
		return m_readpos;
	}

	std::string_view const& string_view() const {
		assert(!m_cstr);
		return m_stdstr;
	}

	yesinline
	char const* data() const {
		return isView() ? m_stdstr.data() : m_cstr;
	}

	yesinline
	bool empty() const {
		if (isView()) { 
			return m_stdstr.empty();
		}
		else {
			return m_cstr && !m_cstr[0];
		}

	}

	int fixed_length() const {
		if (m_cstr) {
			return INT_MAX;
		}

		return m_stdstr.length();
	}
};


using StringViewGenericArg = StringViewSpecificArg<>;
