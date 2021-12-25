
#include "appSettings.h"
#include "appSettingsMap.h"
#include "StringUtil.h"


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
			fprintf(stderr, "Config error: expected boolean r-value parsing %s=%s", name.c_str(), rvalue.c_str());
			return {};
		}
	}
	return {true, result};
}

bool appGetSettingBool(const std::string& name, bool nonexist_result) {
	auto [exists, value] = _getSettingBool(g_map, name);
	return exists ? value : nonexist_result;
}

}
