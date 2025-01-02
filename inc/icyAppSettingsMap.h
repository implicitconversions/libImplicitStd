// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

#include "icyAppSettingsBase.h"
#include <map>
#include <optional>


namespace icyAppSettingsIfc
{
	extern std::map<std::string, std::string> g_map;

	// used by Xem Tooling to implement a custom multi-tier settings system.
	// (todo: probably a better way to implement that system than this, which is a bit sloppy/rushed)
	extern StdOptionString<bool> _getSettingBool(std::map<std::string, std::string> const& map, const std::string& name);
}
