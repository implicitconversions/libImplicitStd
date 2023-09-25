SOURCES_libImplicitStd :=
SOURCES_libImplicitStd += src/UnattendedMode.cpp
SOURCES_libImplicitStd += src/AppSettings.cpp
SOURCES_libImplicitStd += src/ConfigParse.cpp
SOURCES_libImplicitStd += src/fs.cpp
SOURCES_libImplicitStd += src/standardfilesystem.cpp
SOURCES_libImplicitStd += src/StringBuilder.cpp
SOURCES_libImplicitStd += src/StringUtil.cpp
SOURCES_libImplicitStd += src/strtosj.cpp

ifeq ($(platform),msw)
    SOURCES_libImplicitStd += src/pre_main_init_crt.cpp
    SOURCES_libImplicitStd += src/msw_app_console_init.cpp
    SOURCES_libImplicitStd += src/msw_posix_dlsym.cpp
    SOURCES_libImplicitStd += src/msw-printf-stdout.cpp
endif

ifeq ($(m_platform_sce),1)
    SOURCES_libImplicitStd += ../libimplicitstd-sce/src/filesystemSce.cpp
    SOURCES_libImplicitStd += ../libimplicitstd-sce/src/SceErrorCode.cpp
    ifeq ($(HAS_DLSYM),1)
        SOURCES_libImplicitStd += ../libimplicitstd-sce/src/sce_posix_dlsym.cpp
    endif
else
    SOURCES_libImplicitStd += src/filesystem.std.cpp
    SOURCES_libImplicitStd += src/posix_file.cpp
    ifeq ($(HAS_DLSYM),1)
        SOURCES_libImplicitStd += src/posix_dlsym.cpp
    endif
endif
