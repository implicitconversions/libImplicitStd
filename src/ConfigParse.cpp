#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "ConfigParse.h"
#include "StringUtil.h"
#include "fs.h"
#include "defer.h"

#include "icy_log.h"

bool ConfigParseLine(const char* readbuf, const ConfigParseAddFunc& push_item, int linenum) {
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
		log_error("Skipping invalid entry (line %d): %s", linenum, line.c_str());
	}
	return 0;
}

bool ConfigParseFile(FILE* fp, const ConfigParseAddFunc& push_item) {
	constexpr int max_buf = 4096;
	char readbuf[max_buf];
	auto linenum = 0;
	while (fgets(readbuf,max_buf,fp)) {
		linenum++;
		if (!ConfigParseLine(readbuf, push_item, linenum)) {
			return 0;
		}
	}
	return 1;
}

void ConfigParseArgs(int argc, const char* const argv[], const ConfigParseAddFunc& push_item) {
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
