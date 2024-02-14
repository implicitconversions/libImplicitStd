#pragma once

extern bool DiscoverUnattendedSessionFromEnvironment();

extern __selectany char const* const g_env_verbose_name;

#if PLATFORM_MSW
extern __selectany char const* const g_atd_path_default;
extern __selectany char const* const g_env_attach_debugger;
#endif

// find a better home for this. For the moment it's kind of a one-off thing in icystdlib.
#if PLATFORM_MSW
	extern void exitNoCleanup(int exit_code);
#else
#	define exitNoCleanup(exit_code) (fflush(nullptr), _Exit(exit_code))
#endif
