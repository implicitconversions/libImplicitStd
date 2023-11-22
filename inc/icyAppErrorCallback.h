#pragma once

using FnType_IcyAppErrorCallback = bool(char const* filepos, char const* condmsg, char const* fmt, ...);
extern FnType_IcyAppErrorCallback* libimplicit_app_report_error;
