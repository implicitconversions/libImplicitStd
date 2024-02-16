#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <functional>

#include "fs.h"

using ConfigParseAddFunc = std::function<void(const std::string&, const std::string&)>;

struct ConfigParseContext {
	fs::path fullpath;
	int linenum;
};

extern bool ConfigParseLine(const char* readbuf, const ConfigParseAddFunc& push_item, ConfigParseContext const& ctx = {});
extern bool ConfigParseFile(FILE* fp, const ConfigParseAddFunc& push_item, ConfigParseContext const& ctx = {});
extern void ConfigParseArgs(int argc, const char* const argv[], const ConfigParseAddFunc& push_item);
extern void ConfigParseArgs(int argc, const char* const argv[], char const* prefix, const ConfigParseAddFunc& push_item);


extern void ParseArgumentsToArgcArgv(const std::vector<std::string>& arguments, std::function<void(int argc, const char* argv[])> const& callback);

extern char const* ConfigParseSingleArg(char const* input, char const *lvalue, int lvalue_length);

template<int LVALUE_SIZE>
char const* ConfigParseSingleArg(char const* input, char const (&lvalue)[LVALUE_SIZE]) {
	return ConfigParseSingleArg(input, lvalue, LVALUE_SIZE-1);
}

