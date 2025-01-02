// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

#include <cstdio>

// the fflush() bookends are needed on log_error on windows platforms, otherwise stdout and stderr outputs will
// issue out-of-order to the users' console TTY. (unix'y platforms may handle this gracefully behind the scenes?)

#if !defined(log_host)
#   define log_host(fmt, ...)        (                 fprintf(stdout, fmt "\n", ## __VA_ARGS__), fflush(nullptr))
#endif
#if !defined(log_error)
#   define log_error(fmt, ...)       (fflush(nullptr), fprintf(stderr, fmt "\n", ## __VA_ARGS__), fflush(nullptr))
#endif

