SHELL := /bin/bash

TARGET := testapp
VERSION ?= devel

OBJDIR ?= .obj/$(platform)
RUNDIR := .

LIB_IMPLICIT_STD_DIR := .

# to allow make and submake access to bash src/script/script.helpers/proxy tools.
export PATH := $(abspath ./msbuild):$(PATH)

# CPPSTDFLAG allows C+ standard to be easily overridden from CLI
# Default is set as c++2a to avoid requirement of exceedling recent g++. Can revisit updating to 
# C++20 if/when we encounter some need while authoring C++ code.
CPPSTDFLAG ?= -std=c++2a

# delayed eval: eval when referenced.
TARGET_FULLPATH = $(RUNDIR)/$(TARGET)

# redirect libraries built by submake into our .obj dir for convenience.
LDFLAGS = -L$(OBJDIR)

.DEFAULT_GOAL := all

# valid target platforms: ps4, ps5, msw, linux, osx
#  - ps4/ps5 must be specified manually on CLI.
#  - other platforms work by autodettection.

uname_s := $(shell uname -s)
ifneq ($(findstring MINGW,$(uname_s)),) # win (msys2)
    shell_os ?= msw
else ifneq ($(findstring Darwin,$(uname_s)),) # osx
    shell_os ?= osx
else
    shell_os ?= linux
endif

ifeq ($(platform),)
    ifeq ($(shell_os),msw)
        platform ?= msw
    else ifeq ($(shell_os),osx) 
        platform ?= osx
    else
        platform ?= linux
    endif
endif

# differentiate public includes from local includes:
#  - Public includes are referenced by #include <...> and are propagated to sub-makes
#  - Local  includes are referenced by #include "..." and are _not_ propagated to sub-makes

all_includes.macro = $(INCLUDE_FLAGS) $(m_inherited_inc_flags) $(m_expanded_include_dirs_public) $(m_expanded_include_dirs_local) $(m_expanded_force_includes)

m_expanded_include_dirs_public = $(patsubst %,-I%,$(m_include_dirs_public))
m_expanded_include_dirs_local  = $(patsubst %,-iquote%,$(m_include_dirs_local))
m_expanded_force_includes      = $(patsubst %,-include %,$(m_force_includes))

# to avoid recursion on delayed expansion
m_inherited_inc_flags := $(INCLUDE_FLAGS)
override INCLUDE_FLAGS = $(strip $(m_inherited_inc_flags) $(m_expanded_include_dirs_public) $(m_expanded_force_includes))

m_force_includes :=
m_force_includes += inc/fi-platform-defines.h
m_force_includes += inc/fi-CrossCompile.h
m_force_includes += inc/fi-jfmt.h
m_force_includes += samples/fi-compiler-warnings.h

m_include_dirs_local  := . inc_stub
m_include_dirs_public := inc inc_sys

# flags applied to both C and C++
m_ccxx_generic_flags := -Wall -fno-exceptions

# CPPFLAGS in make means "C PreProcessor flags" -NOT- "C++ flags"
# (CPPFLAGS convention in gnu make predates C++, which is why C++ uses CXXFLAGS instead)
CPPFLAGS  = $(patsubst %,-D%,$(DEFINES))
CFLAGS    = $(m_ccxx_generic_flags) -std=c11
CXXFLAGS  = $(m_ccxx_generic_flags) -fno-rtti
CXXFLAGS += $(CPPSTDFLAG)

# Toggle to include or exclude symbols - generally we always want symbols even for optimized builds.
# This ensures syms are available for all devs. Symbols can be stripped later prior to distributing
# binaries to customers.
WANT_SYMBOLS ?= 1
ifeq ($(WANT_SYMBOLS), 1)
    CXXFLAGS += -g
    CFLAGS += -g
endif

WANT_ATTACH_TO_DEBUGGER ?= 1

DEFINES += WANT_ATTACH_TO_DEBUGGER=$(WANT_ATTACH_TO_DEBUGGER)

ifeq ($(platform), msw)  # msw from mingw or clang64 prompt (MSYS2)
    OSLBL = Windows
    TARGET := $(TARGET).exe
    ifneq ($(findstring CLANG64,$(MSYSTEM)),)
        # new style clang64 toolchain provided by MSYS2 (recommended)
        CC  := clang
        CXX := clang++
    endif

    NEED_PREPROCESS_PRAGMA_COMMENT_LIB := 1

    LDFLAGS += -mwindows -static
    DEFINES += _WINDOWS
    LD = $(CXX)

    m_force_includes += inc/fi-msw-printf-redirect.h

    ifeq ($(WANT_SYMBOLS), 1)
        CXXFLAGS += -gcodeview
        CFLAGS   += -gcodeview
    endif
else ifeq ($(platform), osx)
    OSLBL = OSX
    ASANFLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security
    LDFLAGS += -Ldeps/osx_$(shell uname -m)/lib
    LDFLAGS += -lc++
    # could enable this for mac builds, but for the moment there's no use case.
    NEED_PREPROCESS_PRAGMA_COMMENT_LIB := 0
else # linux
    OSLBL = Linux
#   ASANFLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security
    LDFLAGS += -ldl
    LDFLAGS += $(shell pkg-config --libs glfw3)
    LDFLAGS += -lpthread
    LD = $(CXX)

    # could enable this for linux builds, but for the moment there's no use case.
    NEED_PREPROCESS_PRAGMA_COMMENT_LIB := 0
endif

include $(LIB_IMPLICIT_STD_DIR)/SOURCES.libImplicitStd.mk

SOURCES_CXX := samples/tests_main.cpp
SOURCES_CXX += $(addprefix $(LIB_IMPLICIT_STD_DIR)/,$(SOURCES_libImplicitStd))

OBJECTS := $(SOURCES_CXX:.cpp=.o)
OBJECTS += $(SOURCES_C:.c=.o)

# support for stashing all intermediate build artifacts into a subdir to keep workspace cleaner.
# also exports absolute path to OBDIR for use by sub-make.
OBJECTS := $(addprefix $(OBJDIR)/, $(OBJECTS))

COMPILE.cxx := $(CPPFLAGS) $(CXXFLAGS) $(ASANFLAGS)
COMPILE.c := $(CPPFLAGS) $(CFLAGS) $(ASANFLAGS)

# include incremental makefile rules. Provides implementation of INCR_BUILD_MACRO and incr_touch_depfile.
# This must be done after OBJECTS is assigned and before any of our own recipes are implemented.
include $(LIB_IMPLICIT_STD_DIR)/msbuild/inc/incremental_build_support.mk

COMPILE.LD  := $(LDFLAGS)
$(eval $(call INCR_BUILD_MACRO,.,,LD))

COMPILE.CXX := $(all_includes.macro) $(CPPFLAGS) $(CXXFLAGS) $(ASANFLAGS)
COMPILE.C   := $(all_includes.macro) $(CPPFLAGS) $(CFLAGS)   $(ASANFLAGS)

$(eval $(call INCR_BUILD_MACRO,.,,CXX))
$(eval $(call INCR_BUILD_MACRO,.,,C))

mkobjdir = @[[ -d '$(@D)' ]] || mkdir -p '$(@D)'

.PHONY: all clean vcxproj
.NOTPARALLEL: clean

.RECIPEPREFIX = :

all: $(TARGET_FULLPATH)

ifeq ($(NEED_PREPROCESS_PRAGMA_COMMENT_LIB),1)
    include $(LIB_IMPLICIT_STD_DIR)/msbuild/inc/pragma_comment_lib.mk
endif

$(TARGET_FULLPATH): $(OBJECTS) $(INCREMENTAL_DEPS.LD) $(PRAGMA_LIB_DEPS.LD)
:   $(LD) $(OBJECTS) $(LDFLAGS) $(ASANFLAGS) -o $@ 

$(OBJDIR)/%.o: %.cpp $(INCREMENTAL_DEPS.CXX)
:   $(call mkobjdir)
:   $(CXX) -c $< $(COMPILE.CXX) $(g_incr_flags) -o $@
:   $(call incr_touch_depfile)

$(OBJDIR)/%.o: %.c $(INCREMENTAL_DEPS.C)
:   $(call mkobjdir)
:   $(CC) -c $< $(COMPILE.C) $(g_incr_flags) -o $@
:   $(call incr_touch_depfile)


# used for (and by) Visual studio IDE
# TODO: Import UpdateSolutionProjects.sh.
#vcxproj:
#:   @./msbuild/UpdateSolutionProjects.sh [sln_name] $(DEFINES) ---- $(m_include_dirs_public) $(m_include_dirs_local) $(m_force_includes)

clean:
:   rm -rf $(OBJDIR) $(TARGET_FULLPATH)

# empty target to trick other targets into always being rebuilt.
FORCE:

# removes legacy suffix checking for better -d usability
-include $(LIB_IMPLICIT_STD_DIR)/msbuild/inc/disable_legacy_suffixes.mk
