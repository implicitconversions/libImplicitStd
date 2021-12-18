#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "StringUtil.h"
#include "fs.h"
#include "defer.h"

#include "icy_assert.h"
#include "icy_log.h"

using ConfigParseAddFunc = std::function<void(const std::string&, const std::string&)>;

inline bool ConfigParseLine(const char* readbuf, const ConfigParseAddFunc& push_item, int linenum=0) {
	auto trim = [](const std::string& s) {
		// Treat quotes as whitespace when parsing CLI options from files.
		return StringUtil::trim(s," \t\r\n\"");
	};
	auto line = trim(readbuf);

	// skip comments ('#' is preferred, ';' is legacy)
	// Support and Usage of '#' allows for bash/posix style hashbangs (#!something)
	if (line[0] == ';') return 1;
	if (line[0] == '#') return 1;

	// skip empty lines
	if (line.length() == 0)
		return 1;

	auto pos = line.find('=');
	if (pos != line.npos) {
		push_item(trim(line.substr(0, pos)), trim(line.substr(pos + 1)));
		return 1;
	}
	else {
		ICY_LOG_ERROR("Skipping invalid entry (line %d): %s", linenum, line.c_str());

		// Malformed config file settings should never be present in a verified package file.
		// this is a special case where we want a MASTER build to fail outright but it's OK to let any
		// other build type proceed without failure.

		#if BUILD_MASTER
		master_abort("Package configuration is malformed or corrupted.");
		#endif
	}
	return 0;
}

inline void ConfigParseFile(FILE* fp, const ConfigParseAddFunc& push_item) {
	constexpr int max_buf = 4096;
	char readbuf[max_buf];
	auto linenum = 0;
	while (fgets(readbuf,max_buf,fp)) {
		linenum++;
		ConfigParseLine(readbuf, push_item, linenum);
	}
}

inline void ConfigParseFile(const char* path, const ConfigParseAddFunc& push_item) {
	if (!fs::exists(path)) {
		ICY_LOG("[skipped] '%s': not present.", path);
		return;
	}

	if (FILE* fp = fopen(path, "rt")) {
		ICY_LOG("[loading] '%s'", path);
		ConfigParseFile(fp, push_item);
		fclose(fp);
	}
	else {
		house_abort("fopen('%s') failed: file is invalid or inaccessible.", path);
	}
}

inline void ConfigParseArgs(int argc, const char* const argv[], const ConfigParseAddFunc& push_item) {
	// Do not strip quotes when parsing arguments -- the commandline processor (cmd/bash)
	// will have done that for us already.  Any quotes in the command line are intentional
	// and would have been provided by the user by way of escaped quotes. --jstine

	std::string lvalue;
	std::string rvalue;

	for (int i=0; i<argc; ++i) {
		const char* arg = argv[i];
		if (!arg[0]) continue;

		lvalue.clear();
		rvalue.clear();
		int ci = 0;
		while (arg[ci] && arg[ci] != '=') {
			lvalue += arg[ci++];
		}

		if (arg[ci] == '=') {
			rvalue = &arg[ci+1];
		}
		else {
			// allow support for space-delimted parameter assignment.
			if ((i+1 < argc) && !StringUtil::BeginsWith(argv[i+1], "--")) {
				rvalue = argv[i+1];
			}
		}

		// Do not trim spaces from rvalue.  If there are any spaces present then they
		// are present because the user enclosed them in quotes and intends them to be treated
		// as part of the r-value.
		push_item(StringUtil::trim(lvalue), rvalue);
	}
}
