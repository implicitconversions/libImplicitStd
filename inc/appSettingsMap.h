#pragma once

#include "appSettings.h"
#include <map>
#include <optional>


namespace icyAppSettingsIfc
{
	extern std::map<std::string, std::string> g_map;

	// used by Xem Tooling to implement a custom multi-tier settings system.
	// (todo: probably a better way to implement that system than this, which is a bit sloppy/rushed)
	extern std::optional<bool> _getSettingBool(std::map<std::string, std::string> const& map, const std::string& name);
}
