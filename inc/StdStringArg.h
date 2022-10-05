#pragma once

#include <string>

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
struct StringConversionMagick
{
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
