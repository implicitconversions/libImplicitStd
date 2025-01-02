// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

using FnType_IcyAppErrorCallback = bool(char const* filepos, char const* condmsg, char const* fmt, ...);
extern FnType_IcyAppErrorCallback* libimplicit_app_report_error;
