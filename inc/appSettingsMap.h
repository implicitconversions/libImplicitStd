#pragma once

#include "appSettings.h"
#include <map>
#include <tuple>


namespace icyAppSettingsIfc
{
	extern std::map<std::string, std::string> g_map;
	extern std::tuple<bool, bool> _getSettingBool(std::map<std::string, std::string> const& map, const std::string& name);
}
