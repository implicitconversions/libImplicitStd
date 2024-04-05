#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "ConfigParse.h"
#include "StringUtil.h"
#include "fs.h"
#include "defer.h"

#include "icy_log.h"
#include "icy_assert.h"

bool ConfigParseLine(const char* readbuf, const ConfigParseAddFunc& push_item, ConfigParseContext const& ctx) {
	auto trim = [](const std::string& s) {
		// Treat quotes as whitespace when parsing CLI options from files.
		return StringUtil::trim(s," \t\r\n\"");
	};
	auto line = trim(readbuf);

	// skip empty lines to avoid iterator check failures.
	// skip comments ('#' is preferred, ';' is legacy)
	// Support and Usage of '#' allows for bash/posix style hashbangs (#!something)
	if (line.empty())   return 1;
	if (line[0] == ';') return 1;
	if (line[0] == '#') return 1;

	// support for !include "something-else.txt"
	if (line[0] == '!') {
		// Shebang command.
		bool isRequired = line.substr(1, strlen("require")) == "require";
		if (isRequired || line.substr(1, strlen("include")) == "include") {
			auto cwd = ctx.fullpath.dirname();
			auto pos = line.find_first_of(" \t");
			auto include_filename = trim(line.substr(pos + 1 + 1));
			auto include_fullpath = cwd / include_filename;

			if (fs::is_directory(include_fullpath)) {
				log_error("%s(%d): invalid !%s directive, %s is a directory.",
					ctx.fullpath.c_str(), ctx.linenum, isRequired ? "require" : "include", include_fullpath.uni_string().c_str()
				);
				return 0;
			}

			if (FILE* fp = fopen(include_fullpath, "rt")) {
				Defer(fclose(fp));
				log_host("!%s '%s'", isRequired ? "require" : "include", include_fullpath.uni_string().c_str());
				return ConfigParseFile(fp, push_item, {include_fullpath});
			}
			else {
				if (isRequired) {
					auto err = errno;
					log_error("%s(%d): %s could not be opened for reading: %s",
						ctx.fullpath.c_str(), ctx.linenum, include_fullpath.uni_string().c_str(),
						strerror(err)
					);
					return 0;
				}

			}
		}
		return 1;
	}

	auto pos = line.find('=');
	if (pos != line.npos) {
		push_item(trim(line.substr(0, pos)), trim(line.substr(pos + 1)));
		return 1;
	}
	else {
		log_error("%s(%d): expected assignment (=): %s", ctx.fullpath.c_str(), ctx.linenum, line.c_str());
	}
	return 0;
}

bool ConfigParseFile(FILE* fp, const ConfigParseAddFunc& push_item, ConfigParseContext const& ctx) {
	char readbuf[2048];
	auto linenum = 0;
	while (fgets(readbuf, fp)) {
		linenum++;
		if (!ConfigParseLine(readbuf, push_item, { ctx.fullpath, ctx.linenum + linenum })) {
			return 0;
		}
	}
	return 1;
}

void ParseArgumentsToArgcArgv(const std::vector<std::string>& arguments, std::function<void(int argc, const char* argv[])> const& callback) {
	std::vector<const char*> argv;
	for (const auto& argument : arguments) {
		argv.push_back(argument.c_str());
	}
	argv.push_back(NULL);

	callback(argv.size() - 1, argv.data());
}

// Finds the first character after '=' and returns a pointer to that. Only suitable for use
// CLI arguments, because it does not consider quotes or whitespace: whitespace and quotes
// handling is done by the shell invoker, allowing us to take the string literally as provided.
extern char const* ConfigParseSingleArg(char const* input, char const *lvalue, int lvalue_length) {
	if (lvalue_length <= 0) return nullptr;
	if (strncmp(input, lvalue, lvalue_length) == 0) {
		if (input[lvalue_length] == '=') {
			return input + lvalue_length + 1;
		}
		else {
			log_error("CLI: expected assignment (=) for %s", input);
		}
	}
	return nullptr;
}

void ConfigParseArgs(int argc, const char* const argv[], const ConfigParseAddFunc& push_item) {
	return ConfigParseArgs(argc, argv, "--", push_item);
}

void ConfigParseArgs(int argc, const char* const argv[], char const* prefix, const ConfigParseAddFunc& push_item) {
	// Do not strip quotes when parsing arguments -- the commandline processor (cmd/bash)
	// will have done that for us already.  Any quotes in the command line are intentional
	// and would have been provided by the user by way of escaped quotes. --jstine

	std::string lvalue;
	std::string rvalue;

	bool end_of_options = 0;

	for (int i=0; i<argc; ++i) {
		const char* arg = argv[i];
		if (!arg[0]) continue;

		// if you need this function to ignore '--' as an end_of_options flag then pre-process the argv and set
		// any offending naked '--' to null.
		if (strcmp(arg, "--")==0) {
			// rest of the CLI is positional args.
			end_of_options = 1;
			continue;
		}

		if (end_of_options) {
			push_item({}, arg);
			continue;
		}

		lvalue.clear();
		rvalue.clear();
		int ci = 0;
		while (arg[ci] && arg[ci] != '=') {
			lvalue += arg[ci++];
		}

		// anything with an assignment (equals) is considered a valid KVP unless it occurs after end_of_options.
		if (arg[ci] == '=') {
			rvalue = &arg[ci+1];
		}
		else {
			// allow support for space-delimited parameter assignment.
			// valid only if the prefix is non-null and the rvalue has no assignment operator (=).
			if ((i+1 < argc) && (prefix && prefix[0] && argv[i+1] && !StringUtil::BeginsWith(argv[i+1], prefix) && !strchr(argv[i+1], '='))) {
				rvalue = argv[++i];
			}
			else {
				// no assignment operator? treat this as a positional parameter (not an argument or switch)
				push_item({}, arg);
			}
		}

		// Do not trim spaces from rvalue.  If there are any spaces present then they
		// are present because the user enclosed them in quotes and intends them to be treated
		// as part of the r-value.
		push_item(StringUtil::trim(lvalue), rvalue);
	}
}
