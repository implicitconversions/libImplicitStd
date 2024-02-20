# incremental-build-support.mk
#
# Input Requirements
#   INCREMENTAL - must be fully defined prior to inclusion, to allow for correct rules definitions.
#
# Output Definitions:
#   g_incr_flags - pass this into the compiler!
#
# Notes when Authoring Rules/Recipes:
#  * sub-makes must be forced when building incrementally, since only the submake knows its dependencies.
#  * clang on msys2 writes the dep (.d) file after writing the object file, which tricks make into thinking
#    the object is out-of-date. The .d file must be touched after the compile step to force the dep timestamp
#    to match the obj file. Example:
#       $(CXX) -c $< all the stuff ...
#       $(call incr_touch_depfile)

m_incr_mydir := $(dir $(realpath -m $(lastword $(MAKEFILE_LIST))))

# only works on make 4.4 or newer.
#export PATH := $(abspath $(m_incr_mydir)):$(PATH)

# enable incremental builds (checks header dependencies)
# incr=1 is provided as a familiar shorthand on the make CLI.
incr ?= 1
INCREMENTAL ?= $(incr)

# set this to -MD for full system file dependency checks.
# this will add a lot of overhead to gnu makefile startup, especially on very large projects, and is only
# useful if your workflow involves modifying system header files frequently. Note that -MD does not generally
# work for compiler version changwes. A full clean build will still be required to aovid spurious errors.
# For that reason, default to -MMD to favor performance.
INCREMENTAL_MODE_SWITCH ?= -MMD


ifeq ($(INCREMENTAL),1)
    # incr_touch_depfile: clang on msys2 writes the dep (.d) file after writing the object file, which tricks make
    # into thinking the object is out-of-date. We use touch after the compile step to force the dep timestamp to
    # match the obj file.
    # TODO: this might only be needed on MSYS2, maybe try NOP'ing it on linux later...
    incr_touch_depfile = @touch --reference=$@ $(basename $@).d || :

    g_incr_flags   = -MT $@ $(INCREMENTAL_MODE_SWITCH) -MP -MF $(basename $@).d
    m_incr_files   = $(addsuffix .d,$(basename $(OBJECTS)))
endif

# Dependency/Incremental Build Rules. These are only effective if INCREMENTAL=1.
ifeq ($(INCREMENTAL),1)
ifeq ($(m_incremental_objects_rule_is_defined),)
m_incremental_objects_rule_is_defined := 1
$(m_incr_files):
include $(wildcard $(m_incr_files))
endif
endif

# Args :
#   1 - variable name prefix, usually empty or "my_lib_name"
#   2 - library subdir location, can leave empty if same as variable name prefix
#   3 - Compiler command (CC, CXX, LD, etc)
# What it does:
#  1. create a hash of the collection of switches used to compile this rule
#  2. compare contents of existing file with what our current compiler options represent.
#     if they don't match, remove the file to triger a rebuild.
#  3. Generate a rule to match all object files in the subdir to the specified extension.
ifeq ($(INCREMENTAL),1)
    define INCR_BUILD_MACRO

    m_local_subvar := $(1).
    ifeq ($(1),.)
        m_local_subvar := 
    endif

    ifeq ($(2),)
        m_local_subdir := $(1)
    endif

    export tmp_makeincr_COMPILE  := $($(3)) $$($$(m_local_subvar)COMPILE.$(3))
    m_local_objdir  := $$(shell realpath -m --relative-to=$$$$(pwd) $$(OBJDIR)/$$(m_local_subdir))
    m_compile_file  := $$(m_local_objdir)/compile_flags.$(3)
    null := $$(shell $$(m_incr_mydir)/incremental_update_compile_flags.sh $$(m_compile_file))
    ifeq ($(3),LD)
        $$(m_local_subvar)INCREMENTAL_DEPS.$(3) := $$(m_compile_file)
    else
        $$(m_local_subvar)INCREMENTAL_DEPS.$(3) := $$(m_compile_file) $$(m_local_objdir)/%.d
    endif
    endef
endif
