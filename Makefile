SHELL := /bin/bash

TARGET := syrup
VERSION ?= devel

OBJDIR ?= .obj/$(platform)
RUNDIR := .

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


LIBRETRO_SCRIPT = $(OBJDIR)/libretro_script.a
LDFLAGS += -lretro_script

# differentiate public includes from local includes:
#  - Public includes are referenced by #include <...> and are propagated to sub-makes
#  - Local  includes are referenced by #include "..." and are _not_ propagated to sub-makes

m_expanded_include_dirs_public = $(patsubst %,-I%,$(m_include_dirs_public))
m_expanded_include_dirs_local  = $(patsubst %,-iquote%,$(m_include_dirs_local))
m_expanded_force_includes      = $(patsubst %,-include %,$(m_force_includes))

# to avoid recursion on delayed expansion
m_inherited_inc_flags := $(INCLUDE_FLAGS)
override INCLUDE_FLAGS = $(strip $(m_inherited_inc_flags) $(m_expanded_include_dirs_public) $(m_expanded_force_includes))

m_force_includes := forceincludes/fi-CompilerWarnings.h forceincludes/fi-cstdlib.h

m_include_dirs_local := . forceincludes

# GGPO is not yet sub-make, so all of its includes are local.
m_include_dirs_local += ggpo/src/lib/ggpo ggpo/src/lib/ggpo/network ggpo/src/lib/ggpo/backends
m_include_dirs_public := \
    include_public \
    deps/include \
    ggpo/src/include \
    libretro_script/include \
    libretro_script/deps \
    libretro_script/deps/lua-5.4.3 \
    tracy \
    tracy/tracy

# flags applied to both C and C++
m_ccxx_generic_flags := -Wall -fno-exceptions -I./src

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
    CPPFLAGS += -g
endif

ifeq ($(platform), ps5)
    OSLBL = ps5
    TARGET := $(TARGET).elf

    ifeq ($(SDK_ROOT),)
        SDK_ROOT := $(SCE_PROSPERO_SDK_DIR)
    endif

    ifeq ($(SDK_ROOT),)
        $(error SDK_ROOT or SCE_PROSPERO_SDK_DIR is required when building platform=ps5.)
    endif

    CC = sdk_root_proxy.sh host_tools/bin/prospero-clang.exe
    CXX = $(CC)
    LD = sdk_root_proxy.sh host_tools/bin/prospero-lld.exe
    AR = sdk_root_proxy.sh host_tools/bin/prospero-llvm-ar.exe
    WAVE = sdk_root_proxy.sh host_tools/bin/prospero-wave-psslc.exe
    CPPFLAGS += -D"RETRO_API=__declspec(dllexport)"
    LDFLAGS += -lScePad_stub_weak -lSceUserService_stub_weak -lSceAgcDriver_stub_weak -lSceAgc_stub_weak -lSceVideoOut_stub_weak
    LDFLAGS += -lSceAudioOut_stub_weak -lSceNpUniversalDataSystem_stub_weak
    LDFLAGS += -lSceSysmodule_stub_weak -lScePosix_stub_weak -lSceNet_stub_weak -lSceNetCtl_stub_weak -lSceSaveData_stub_weak

    # include nosubmission libs separately. These will need to be conditionally excluded later for the master candidiate (submission) build target
    LDFLAGS += -lSceAgc_debug_nosubmission -lSceAgcCore_debug_nosubmission -lSceAgcGpuAddress_debug_nosubmission -lSceAgcGnmp_debug_nosubmission 
    LDFLAGS += -lSceNet_nosubmission_stub_weak 

    # some programs in the SCE Toolchain expect SCE_PROSPERO_SDK_DIR to be set and may behave in undefined ways when it's missing.
    export SCE_PROSPERO_SDK_DIR := $(SDK_ROOT)
    export SDK_ROOT
else
    ifeq ($(platform), msw)  # msw from mingw or clang64 prompt (MSYS2)
        OSLBL = Windows
        TARGET := $(TARGET).exe
        ifneq ($(findstring CLANG64,$(MSYSTEM)),)
            # new style clang64 toolchain provided by MSYS2 (recommended)
            CC  := clang
            CXX := clang++
        endif
        LDFLAGS += -lglfw3 -lportaudio -mwindows -static
        LDFLAGS += -lpthread -lws2_32 -lwinmm -lOle32 -lSetupAPI
        DEFINES += _WINDOWS
        #Flags for Tracy, also needs ws2_32 but that is already linked for GGPO
        LDFLAGS += -ldbghelp -ladvapi32 -luser32
        LD = $(CXX)
    else ifeq ($(platform), osx)
        OSLBL = OSX
        ASANFLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security
        LDFLAGS += -Ldeps/osx_$(shell uname -m)/lib -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit
        LDFLAGS += -lc++
    else # linux
        OSLBL = Linux
    #   ASANFLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security
        LDFLAGS += -ldl
        LDFLAGS += $(shell pkg-config --libs glfw3)
        LDFLAGS += -lportaudio
        LDFLAGS += -lpthread
        LD = $(CXX)
    endif
endif

SOURCES_CXX = src/main.cpp \
	src/audio/audio_capture.cpp \
	src/config/config.cpp \
	src/core/core.cpp \
	deps/ini.cpp \
	src/netplay/netplay.cpp \
	src/core/options.cpp \
	src/utils/utils.cpp \
	src/script/script.cpp \
	src/video/video_common.cpp \
	src/video/video_common_autobatch.cpp \
	src/audio/audio_resampler_hermite.cpp \
	src/audio/wav16.cpp

ifeq ($(platform), ps5)
SOURCES_CXX += src/plat/plat_sce.cpp \
	src/utils/allocator.cpp \
	src/utils/block_allocator.cpp \
	src/plat/dynlib_sce.cpp \
	src/audio/audio_ps5.cpp \
	src/video/video_ps5.cpp \
	src/input/input_ps5.cpp \
	src/save/srm_ps5.cpp \
	src/trophies/trophies_ps5.cpp
SOURCES_PSSL = src/video/pssl/shader_default_p.pssl src/video/pssl/shader_zfastcrt_p.pssl src/video/pssl/shader_zfastlcd_p.pssl src/video/pssl/shader_default_vv.pssl
DEFINES += SINGLE_THREADED
else
SOURCES_CXX += src/plat/plat_glfw.cpp \
	deps/glad.cpp \
	src/audio/audio_pa.cpp \
	src/video/video_gl.cpp \
	src/input/input_glfw.cpp \
	src/save/srm.cpp \
	src/trophies/trophies_dummy.cpp
# Tracy client and tracy enable flag
SOURCES_CXX += tracy/TracyClient.cpp
DEFINES += TRACY_ENABLE
endif

SOURCES_CXX += ggpo/src/lib/ggpo/bitvector.cpp \
	ggpo/src/lib/ggpo/game_input.cpp \
	ggpo/src/lib/ggpo/input_queue.cpp \
	ggpo/src/lib/ggpo/log.cpp \
	ggpo/src/lib/ggpo/ggpomain.cpp \
	ggpo/src/lib/ggpo/holepunch.cpp \
	ggpo/src/lib/ggpo/poll.cpp \
	ggpo/src/lib/ggpo/sync.cpp \
	ggpo/src/lib/ggpo/timesync.cpp \
	ggpo/src/lib/ggpo/pevents.cpp \
	ggpo/src/lib/ggpo/network/udp.cpp \
	ggpo/src/lib/ggpo/network/udp_proto.cpp \
	ggpo/src/lib/ggpo/backends/p2p.cpp \
	ggpo/src/lib/ggpo/backends/spectator.cpp \
	ggpo/src/lib/ggpo/backends/synctest.cpp

# on windows we need to add platform_windows.cpp to the list of sources
ifeq ($(platform),msw)
    SOURCES_CXX += ggpo/src/lib/ggpo/platform_windows.cpp
else ifeq ($(platform), ps5)
    SOURCES_CXX += ggpo/src/lib/ggpo/platform_sce.cpp
else
    SOURCES_CXX += ggpo/src/lib/ggpo/platform_unix.cpp
endif

OBJECTS := $(SOURCES_CXX:.cpp=.o)
OBJECTS += $(SOURCES_C:.c=.o)

ifeq ($(platform), ps5)
    OBJECTS += $(SOURCES_PSSL:.pssl=.o)
endif

# support for stashing all intermediate build artifacts into a subdir to keep workspace cleaner.
# also exports absolute path to OBDIR for use by sub-make.
OBJECTS := $(addprefix $(OBJDIR)/, $(OBJECTS))

COMPILE.cxx := $(CPPFLAGS) $(CXXFLAGS) $(ASANFLAGS)

# include incremental makefile rules. This must be done after OBJECTS is assigned and before
# any of our own recipes are implemented.
include msbuild/incremental-build-support.mk

mkobjdir = @[[ -d '$(@D)' ]] || mkdir -p '$(@D)'

# usage: $(call submake,library_dir,target,relative_dir_fixup)
#  $1 - library_dir is the directory of the library to submake-invoke.
#  $2 - target is the target name to build (empty is ok if the makefile has suitable default)
#  $3 - relative_dir_fixup is some number of ../.. needed to get from the library dir back to the
#       root dir of the caller makefile (this one)
#  $4 - Any extra arguments, if needed.
submake = $(MAKE) INCLUDE_FLAGS="$(shell ./msbuild/submake_relative_includes.sh $1 $(INCLUDE_FLAGS))"
submake +=        -I$3/msbuild OBJDIR="$3/$(OBJDIR)/$1" $4 -C $1 $2

# ensure sub-make invocations do things the way we expect.
# (once we export vars, they become immutable)
export platform
export CC
export CXX
export AR
export INCREMENTAL
export CPPSTDFLAG
export SDK_ROOT

ifeq ($(INCREMENTAL),1)
    m_force_submake := FORCE
else
    m_force_submake := 
endif

.PHONY: all clean rebuild vcxproj
.NOTPARALLEL: clean rebuild

.RECIPEPREFIX = :

all: $(TARGET_FULLPATH)

$(TARGET_FULLPATH): $(OBJECTS) $(LIBRETRO_SCRIPT) $(m_compile_file.cxx)
:   $(LD) -o $@ $(OBJECTS) $(LDFLAGS) $(ASANFLAGS)

# the -profile of wave-psslc is inferred automatically from the _vv or _p postfix on the shader filenames.
$(OBJDIR)/%.o: %.pssl $(g_incr_prereqs)
:   $(call mkobjdir)
:   $(WAVE) $< -o $(OBJDIR)/$*.agc $(g_incr_flags)
:   $(WAVE) -ags2o $(OBJDIR)/$*.agc -o $@


# Incremental Build Notes:
#  * sub-makes must be forced when building incrementally, since only the submake knows its dependencies.
#  * clang on msys2 writes the dep (.d) file after writing the object file, which tricks make into thinking the object is
#    out-of-date. We use touch after the compile step to force the dep timestamp to match the obj file. 
$(OBJDIR)/%.o: %.cpp $(g_incr_prereqs) $(m_compile_file.cxx)
:   $(call mkobjdir)
:   $(CXX) -c $< $(INCLUDE_FLAGS) $(m_expanded_include_dirs_local) $(COMPILE.cxx) $(g_incr_flags) -o $@
:   $(call incr_touch_depfile)

# Submakes Section
#   + means 'execute this command under make -n', needed because make cannot implicitly know to add this
#   directive due to the call syntax. (it normally does this for us when using $(MAKE) directly in a recipe)

# This rule is provided to catch "single file compile" requests from IDEs.
$(OBJDIR)/libretro_script/%.o:
:   +$(call submake,libretro_script,../$@,..)

$(LIBRETRO_SCRIPT): $(m_force_submake)
:   +$(call submake,libretro_script,lib,..,LIB=../$(LIBRETRO_SCRIPT))

# used for (and by) Visual studio IDE
vcxproj:
:   @./msbuild/UpdateSolutionProjects.sh $(m_include_dirs_local) $(m_include_dirs_public) $(m_force_includes)
 

clean-script:
:   +$(call submake,libretro_script,clean-lib,..)

clean:
:   +$(call submake,libretro_script,clean,..)
:   rm -rf $(OBJDIR) $(TARGET_FULLPATH) Syrup-* *.ags

# performs equivalent of make clean && make all
rebuild: | clean all

# empty target to trick other targets into always being rebuilt.
FORCE:

# removes legacy suffix checking for better -d usability
-include msbuild/disable-legacy-suffixes.mk
