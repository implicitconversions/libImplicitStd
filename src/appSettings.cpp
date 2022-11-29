
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

std::string appGetSetting(const std::string& name) {
	auto it = m_map.find(name);
	if (it == m_map.end()) return {};
	return it->second;
}

std::tuple<std::string, bool> appGetSettingTuple(const std::string& name) {
	auto it = m_map.find(name);
	if (it == m_map.end()) return {};
	return { it->second, true };
}

std::string appGetSettingLwr(const std::string& name) {
	auto it = m_map.find(name);
	if (it == m_map.end()) return {};
	return StringUtil::toLower(it->second);
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
std::tuple<bool, bool> _getSettingBool(std::map<std::string, std::string> const& map, const std::string& name) {
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
			fprintf(stderr, "Config error: expected boolean r-value parsing %s=%s\n", name.c_str(), rvalue.c_str());
			return {};
		}
	}
	return {true, result};
}

bool appGetSettingBool(const std::string& name, bool nonexist_result) {
	auto [exists, value] = _getSettingBool(g_map, name);
	return exists ? value : nonexist_result;
}

uint32_t appGetSettingUint32(const std::string& name, uint32_t nonexist_result) {
	if (auto val = appGetSetting(name); !val.empty()) {
		auto result = strtouj(val.c_str());
		if (result != (uint32_t)result) {
			errno = ERANGE;
			return UINT32_MAX;
		}
	}
	return nonexist_result;
}

ptrdiff_t appGetSettingMemorySize(const std::string& name, ptrdiff_t nonexist_result) {
	std::string val;
	if (appGetSetting(name, val)) {
		char *endptr = nullptr;
		const char* valstr = val.c_str();
		auto value = strtod(valstr, &endptr);
		if (value <= 0 || endptr == valstr) {
			fprintf(stderr, "Config error: expected size argument when parsing %s=%s [ex: 30.5mib, 3000kib, 1280000 (bytes)]\n", name.c_str(), val.c_str());
			return nonexist_result;
		}
		elif (auto scalar = CvtNumericalPostfixToScalar(endptr); scalar >= 0) {
			return value * scalar;
		}
		else {
			fprintf(stderr, "Expected size postfix when parsing %s=%s [ex: kb,kib,mb,mib]\n", name.c_str(), val.c_str());
			return nonexist_result;
		}
	}
	return nonexist_result;
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

} // namespace
