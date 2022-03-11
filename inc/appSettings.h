#pragma once

#include <string>

// helper for C++17's compound (assignment;conditional) style if() statements. Always returns TRUE
// but overrides an empty string with a default string, hence always returning true. Example:
//   if (auto path = appGetSetting("--assets-dir"); TakeStringOrDefault(path, "assets")) { }
#define TakeStringOrDefault(val, def)  (!val.empty() || ((val = def), 1))


// this is stuck inside of a namespace to make it aeasier to override behavior without having to define a new API.
namespace icyAppSettingsIfc
{
	// Caller is responsible for thread locking.

	extern void			appSetSetting		(const std::string& lvalue, std::string rvalue);
	extern void			appRemoveSetting	(const std::string& lvalue);
	extern std::string	appGetSetting		(const std::string& name);
	extern std::string	appGetSettingLwr	(const std::string& name);
	extern bool			appGetSetting		(const std::string& name, std::string& value);
	extern bool			appHasSetting		(const std::string& name);
	extern bool			appGetSettingBool	(const std::string& name, bool nonexist_result=0);

	extern std::tuple<std::string, bool> appGetSettingTuple(const std::string& name);
	extern bool appSettingDeprecationCheck(std::string const& name, std::string const& deprecated_alias);
};
