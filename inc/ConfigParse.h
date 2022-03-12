#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "fs.h"

using ConfigParseAddFunc = std::function<void(const std::string&, const std::string&)>;

struct ConfigParseContext {
	fs::path fullpath;
	int linenum;
};

extern bool ConfigParseLine(const char* readbuf, const ConfigParseAddFunc& push_item, ConfigParseContext const& ctx = {});
extern bool ConfigParseFile(FILE* fp, const ConfigParseAddFunc& push_item, ConfigParseContext const& ctx = {});
extern void ConfigParseArgs(int argc, const char* const argv[], const ConfigParseAddFunc& push_item);
