# Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

# If you change the file be sure to update libImplicitStd.props as well

SOURCES_libImplicitStd :=
SOURCES_libImplicitStd += src/UnattendedMode.cpp
SOURCES_libImplicitStd += src/AppSettings.cpp
SOURCES_libImplicitStd += src/ConfigParse.cpp
SOURCES_libImplicitStd += src/fs.cpp
SOURCES_libImplicitStd += src/standardfilesystem.cpp
SOURCES_libImplicitStd += src/StringBuilder.cpp
SOURCES_libImplicitStd += src/StringUtil.cpp
SOURCES_libImplicitStd += src/strtosj.cpp
SOURCES_libImplicitStd += src/icyReportError.cpp

ifeq ($(platform),msw)
    SOURCES_libImplicitStd += src/directlink/msw-pre_main_init_crt.cpp
    SOURCES_libImplicitStd += src/msw-app_console_init.cpp
    SOURCES_libImplicitStd += src/msw-posix_dlsym.cpp
    SOURCES_libImplicitStd += src/msw-printf-stdout.cpp
endif

SOURCES_libImplicitStd += src/filesystem.std.cpp
SOURCES_libImplicitStd += src/posix_file.cpp

# HAS_DLSYM should only be set TRUE for Linux/Posix OS.
ifeq ($(HAS_DLSYM),1)
    SOURCES_libImplicitStd += src/posix_dlsym.cpp
endif
