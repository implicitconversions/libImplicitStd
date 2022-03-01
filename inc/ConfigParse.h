#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "fs.h"

using ConfigParseAddFunc = std::function<void(const std::string&, const std::string&)>;

extern bool ConfigParseLine(const char* readbuf, const ConfigParseAddFunc& push_item, int linenum=0);
extern bool ConfigParseFile(FILE* fp, const ConfigParseAddFunc& push_item);
extern void ConfigParseArgs(int argc, const char* const argv[], const ConfigParseAddFunc& push_item);
