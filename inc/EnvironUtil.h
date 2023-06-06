#pragma once

extern bool DiscoverUnattendedSessionFromEnvironment();

extern __selectany char const* const g_env_verbose_name;

#if PLATFORM_MSW
extern __selectany char const* const g_atd_path_default;
extern __selectany char const* const g_env_attach_on_start;
#endif
