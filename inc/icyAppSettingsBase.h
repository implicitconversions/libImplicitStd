// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

// Usage Notes:
//   Including this file directly is not recommended. It should be included through a proxy header that
//   either exposes the base namespace globally, or wraps it in a desired structured class.

#include "StdStringArg.h"
#include "StdStringEmpty.h"
#include "StringTokenizer.h"

#include <string>
#include <optional>
#include <algorithm>
#include <limits>
#include <vector>

// helper for C++17's compound (assignment;conditional) style if() statements. Always returns TRUE. Example:
//   if (auto path = appGetSetting("--assets-dir"); TakeStringOrDefault(path, "assets")) { }
#define TakeStringOrDefault(val, def)  (!val.empty() || ((val = def), true))

MSC_WARNING_DISABLE_PUSH()
MSC_WARNING_DISABLE(4324)		// structure was padded due to alignment specifier, happens on SIMD types (float4, int4)

template<typename T>
class StdOptionString : public std::optional<std::pair<std::optional<T>, std::string>> {
private:
	using _MyBase_ = std::optional<std::pair<std::optional<T>, std::string>>;

public:
	using _MyBase_::_MyBase_;

	T value() const {
		return _MyBase_::value().first.has_value() ? _MyBase_::value().first.value() : T{};
	}

	T value_or(T const& other) const {
		return (_MyBase_::has_value() && _MyBase_::value().first) ? _MyBase_::value().first.value() : other;
	}

	bool empty() const {
		return !_MyBase_::has_value() || _MyBase_::value().second.empty();
	}

	operator bool() const {
		return !empty();
	}

	char const* c_str() const {
		return _MyBase_::has_value() ? _MyBase_::value().second.c_str() : nullptr;
	}

	std::string string() const {
		if (_MyBase_::has_value())
			return _MyBase_::value().second;
		else {
			return {};
		}
	}
};
MSC_WARNING_DISABLE_POP()

// this is stuck inside of a namespace to make it easier to override behavior without having to define a new API.
namespace icyAppSettingsIfc
{
	// Caller is responsible for thread locking.

	extern void			appSetSetting			(const std::string& lvalue, std::string rvalue);
	extern void			appRemoveSetting		(const std::string& lvalue);
	extern std::string  appGetSetting			(StdStringTempArg name);
	extern std::string  appGetSettingLwr		(StdStringTempArg name);
	extern bool			appGetSetting			(const std::string& name, std::string& value);
	extern bool			appHasSetting			(const std::string& name);

	extern bool			appGetSettingBool		(const std::string& name, bool nonexist_result=0);
	extern StdOptionString<ptrdiff_t>	appGetSettingMemorySize	(const std::string& name, ptrdiff_t nonexist_result=0);

	extern std::tuple<std::string, bool> appGetSettingTuple(const std::string& name);
	extern bool appSettingDeprecationCheck(std::string const& name, std::string const& deprecated_alias);

	template<typename T>
	StdOptionString<T> ConvertFromString(std::string const& rval);

	template<typename T>
	StdOptionString<T> appGetSettingOpt(StdStringTempArg name) {
		auto rval = appGetSetting(name);
		if (rval.empty()) {
			return {};
		}
		return icyAppSettingsIfc::ConvertFromString<T>(rval);
	}

	template<typename T>
	bool appEmplaceSetting(StdStringTempArg name, T& outDest, T const& defValue) {
		auto rval = appGetSetting(name);
		if (rval.empty()) {
			outDest = defValue;
			return false;
		}
		auto cvtResult = icyAppSettingsIfc::ConvertFromString<T>(rval);
		if (!cvtResult) {
			outDest = defValue;
			return false;
		}
		return cvtResult.value();
	}

	template<typename T>
	bool appEmplaceSetting(StdStringTempArg name, T& outDest) {
		auto rval = appGetSetting(name);
		if (rval.empty()) {
			return false;
		}
		auto cvtResult = icyAppSettingsIfc::ConvertFromString<T>(rval);
		if (!cvtResult) {
			return false;
		}
		outDest = cvtResult.value();
		return true;
	}

	/**
	 * Returns a vector of values for a setting split by the given delimiter.
	 * For example, calling with ("1920x1280", 'x'), will return { 1920, 1280 }.
	 */
	template<typename T>
	bool appGetSettingDelimited(const std::string& name, char delimeter, std::vector<T>& outDest) {
		auto rval = appGetSetting(name);
		if (rval.empty()) {
			return false;
		}

		outDest.clear();
		auto toks = Tokenizer(rval.c_str());
		while (auto tok = toks.GetNextTokenTrim(delimeter)) {
			auto value = ConvertFromString<T>(tok);
			outDest.push_back(value.value());
		}

		return true;
	}

	namespace _template_impl {
		template<bool isSigned>
		auto _strtoj_tmpl(char const* src, char** endptr=nullptr, int radix=0) {
			if constexpr(isSigned) {
				return strtosj(src, endptr, radix);
			}
			else {
				return strtouj(src, endptr, radix);
			}
		}

		extern StdOptionString<bool> ConvertToBool(std::string const& rval);

		template<typename T>
		StdOptionString<T> ConvertFromString_integrals(std::string const& rval) {
			if (rval.empty()) {
				return {};
			}

			auto result = _strtoj_tmpl<std::is_signed_v<T>> (rval.c_str());
			if (result != (T)result) {
				errno = ERANGE;
				return std::pair { std::clamp<decltype(result)>(result, std::numeric_limits<T>::min(), std::numeric_limits<T>::max()), rval };
			}
			return std::pair { result, rval };
		}

		StdOptionString<double> ConvertFromString_f64(std::string const& rval);
		StdOptionString<float > ConvertFromString_f32(std::string const& rval);
	}
};
