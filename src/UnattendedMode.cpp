
#include <cstdlib>
#include "EnvironUtil.h"

// Returns true if the current running environment hints at the idea that this application will
// be unattended by humans (eg, automated or non-interactive). The application should endeavor
// to avoid popups at all costs in this case.
bool DiscoverUnattendedSessionFromEnvironment() {
#if PLATFORM_HAS_GETENV
	if(auto* rvalue = getenv("UNATTENDED"); rvalue && rvalue[0]) {
		return rvalue[0] != '0';
	}

	if(auto* rvalue = getenv("AUTOMATED"); rvalue && rvalue[0]) {
		return rvalue[0] != '0';
	}

	// very oldschool and very long to type, don't think anyone uses this one.
	//if(auto* rvalue = getenv("NONINTERACTIVE"); rvalue && rvalue[0]) {
	//	return rvalue[0] != '0';
	//}

	// Jenkins has room for false positives where JENKINS_HOME is set but it's just some
	// interactive shell environment and not part of the automated job process.
	if (const auto* rvalue = getenv("JENKINS_HOME"); rvalue && rvalue[0]) {
		auto* node_name = getenv("NODE_NAME");
		auto* job_name  = getenv("BUILD_TAG");
		return 
			node_name    && job_name &&
			node_name[0] && job_name[0]
		;
	}

	if (const auto* rvalue = getenv("GITLAB_CI"); rvalue && rvalue[0]) {
		return 1;
	}

	if (const auto* rvalue = getenv("GITHUB_ACTIONS"); rvalue && rvalue[0]) {
		return 1;
	}
#endif

	return 0;
}
