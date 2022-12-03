
#include "appSettings.h"
#include "appSettingsMap.h"
#include "StringUtil.h"
#include "icy_log.h"

namespace icyAppSettingsIfc
{
std::map<std::string, std::string> g_map;

// lazy way to make this code look like a class member, and just in case we want to class it later.
static auto& m_map = g_map;

void appSetSetting(const std::string& lvalue, std::string rvalue) {
	m_map[lvalue] = rvalue;
}

void appRemoveSetting(const std::string& lvalue) {
	m_map.erase(lvalue);
}

std::string appGetSetting(StdStringTempArg name) {
	auto it = m_map.find(name);
	if (it == m_map.end()) return {};
	return it->second;
}

std::tuple<std::string, bool> appGetSettingTuple(const std::string& name) {
	auto it = m_map.find(name);
	if (it == m_map.end()) return {};
	return { it->second, true };
}

std::string appGetSettingLwr(StdStringTempArg name) {
	return StringUtil::toLower(appGetSetting(name));
}

bool appGetSetting(const std::string& name, std::string& value) {
	auto it = m_map.find(name);
	if (it == m_map.end())
		return false;

	value = it->second;
	return true;
}

bool appHasSetting(const std::string& name) {
	return m_map.find(name) != m_map.end();
}

// returns 'exists' and 'value'
std::optional<bool> _getSettingBool(std::map<std::string, std::string> const& map, const std::string& name) {
	auto it = map.find(name);
	if (it == map.end()) return {};

	// switch which is present but has no assgned value is assumed 1.
	// in this way --do-a-thing will be interpreted as --do-a-thing=1
	// All switches should be designed to adhere to this pattern.

	bool result = 1;
	if (auto rvalue = it->second; !rvalue.empty()) {
		bool parse_error;
		result = StringUtil::getBoolean(rvalue, &parse_error);
		if (parse_error) {
			errno = EINVAL;
			fprintf(stderr, "Config error: expected boolean r-value parsing %s=%s\n", name.c_str(), rvalue.c_str());
			return {};
		}
	}
	return result;
}

bool appGetSettingBool(const std::string& name, bool nonexist_result) {
	auto opt = _getSettingBool(g_map, name);
	return opt.value_or(nonexist_result);
}

StdOptionString<ptrdiff_t> appGetSettingMemorySize(const std::string& name, ptrdiff_t nonexist_result) {
	auto rval = appGetSetting(name);
	auto result = std::pair { nonexist_result, rval };

	if (rval.empty()) {
		return result;
	}

	char *endptr = nullptr;
	const char* valstr = rval.c_str();
	auto value = strtod(valstr, &endptr);
	if (value <= 0 || endptr == valstr) {
		fprintf(stderr, "Config error: expected size argument when parsing %s=%s [ex: 30.5mib, 3000kib, 1280000 (bytes)]\n", name.c_str(), rval.c_str());
		errno = EINVAL;
	}
	elif (auto scalar = CvtNumericalPostfixToScalar(endptr); scalar >= 0) {
		result.first = value * scalar, rval;
	}
	else {
		fprintf(stderr, "Expected size postfix when parsing %s=%s [ex: kb,kib,mb,mib]\n", name.c_str(), rval.c_str());
		errno = EINVAL;
	}
	return result;
}

bool appSettingDeprecationCheck(std::string const& name, std::string const& deprecated_alias) {
	if (auto [depr_value, depr_exists] = appGetSettingTuple(deprecated_alias); depr_exists) {
		if (!appHasSetting(name)) {
			appSetSetting(name, depr_value);
		}
		return true;
		//log_host("CLI Option '%s' is deprecated. Please use '%s' instead.", name.c_str());
	}
	return false;
}

std::optional<bool> _template_impl::ConvertToBool(std::string const& rval) {
	return _getSettingBool(g_map, rval);
}

std::optional<double> _template_impl::ConvertFromString_f64(std::string const& rval) {
	char *endptr = nullptr;
	const char* valstr = rval.c_str();
	auto value = strtod(valstr, &endptr);
	if (endptr == valstr) {
		errno = EINVAL;
		return {};
	}
	return value;
}

std::optional<float> _template_impl::ConvertFromString_f32(std::string const& rval) {
	char *endptr = nullptr;
	const char* valstr = rval.c_str();
	auto value = strtof(valstr, &endptr);
	if (endptr == valstr) {
		errno = EINVAL;
		return {};
	}
	return value;
}

template<> std::optional<bool    > ConvertFromString(std::string const& rval) { return _template_impl::ConvertToBool(rval); }
template<> std::optional<uint32_t> ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_integrals<uint32_t>(rval); }
template<> std::optional< int32_t> ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_integrals< int32_t>(rval); }
template<> std::optional<uint64_t> ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_integrals<uint64_t>(rval); }
template<> std::optional< int64_t> ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_integrals< int64_t>(rval); }
template<> std::optional<uint16_t> ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_integrals<uint16_t>(rval); }
template<> std::optional< int16_t> ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_integrals< int16_t>(rval); }
template<> std::optional<uint8_t > ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_integrals<uint8_t >(rval); }
template<> std::optional< int8_t > ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_integrals< int8_t >(rval); }
template<> std::optional<float>    ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_f32(rval); }
template<> std::optional<double>   ConvertFromString(std::string const& rval) { return _template_impl::ConvertFromString_f64(rval); }
} // namespace



