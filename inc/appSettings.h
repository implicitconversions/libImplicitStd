#pragma once

#include <string>

// this is stuck inside of a namespace to make it aeasier to override behavior without having to define a new API.

namespace icyAppSettingsIfc
{
	// Caller is responsible for thread locking.

	extern void			appSetSetting		(const std::string& lvalue, const std::string& rvalue);
	extern void			appRemoveSetting	(const std::string& lvalue);
	extern std::string	appGetSetting		(const std::string& name);
	extern std::string	appGetSettingLwr	(const std::string& name);
	extern bool			appGetSetting		(const std::string& name, std::string& value);
	extern bool			appHasSetting		(const std::string& name);
	extern bool			appGetSettingBool	(const std::string& name, bool nonexist_result=0);
};
