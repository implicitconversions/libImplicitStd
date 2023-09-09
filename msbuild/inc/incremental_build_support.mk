# incremental-build-support.mk
#
# Required Input Definitions:
#   OBJDIR  - dir in which objects and intermediate build items are stored.
#   OBJECTS - the complete list of object files needed to build the primary target.
#   COMPILE.cxx
#   COMPILE.c
#
# Output Definitions:
#   g_incr_flags - pass this into the compiler!
#   g_incr_prereqs - this is a dependency for the compiler recipe
#
# Notes when Authoring Rules/Recipes:
#  * sub-makes must be forced when building incrementally, since only the submake knows its dependencies.
#  * clang on msys2 writes the dep (.d) file after writing the object file, which tricks make into thinking
#    the object is out-of-date. The .d file must be touched after the compile step to force the dep timestamp
#    to match the obj file. Example:
#       $(CXX) -c $< all the stuff ...
#       $(call incr_touch_depfile)

# enable incremental builds (checks header dependencies)
# incr=1 is provided as a familiar shorthand on the make CLI.
incr ?= 1
INCREMENTAL ?= $(incr)

# TODO: this might only be needed on MSYS2, maybe try NOP'ing it on linux later...
incr_touch_depfile = @(( $(INCREMENTAL) )) && touch --reference=$@ $(OBJDIR)/$*.d || :

ifeq ($(INCREMENTAL),1)
    g_incr_flags   = -MT $@ -MD -MP -MF $(OBJDIR)/$*.d
    g_incr_prereqs = $(OBJDIR)/%.d
    m_incr_files   = $(OBJECTS:%.o=%.d)
endif

# Dependency/Incremental Build Rules. These are olny effective if INCREMENTAL=1.
$(m_incr_files):
include $(wildcard $(m_incr_files))

### Dependency Recompiler based on compiler options ###

#  1. create a hash of the collection of switches used to compile this rule
#  2. compare contents of existing file with what our current compiler options represent.
#     if they don't match, remove the file to triger a rebuild.

ifeq ($(INCREMENTAL),1)

    m_link_incr_file  ?= $(OBJDIR)/linker_ldflags
    ifneq ($(LDFLAGS),)
        export LDFLAGS
        m_hash_linker  = $(shell sha256sum <<< "$$LDFLAGS" | cut -c-64)
        ifeq ($(shell cmp -s $(m_link_incr_file) <(echo "$(m_hash_linker)") || echo 1),1)
            null := $(shell rm -f $(m_link_incr_file))
        endif
    endif

    # export is required to avoid make mangling the string which contains quotes and parens.
    export makeincr_COMPILE_cxx  := $(CXX) $(COMPILE.cxx)
    export makeincr_COMPILE_c    := $(CC)  $(COMPILE.c)
    export makeincr_COMPILE_pssl := $(COMPILE.pssl)

    m_compile_file.cxx  ?= $(OBJDIR)/cxx.compile_flags
    m_compile_file.c    ?= $(OBJDIR)/c.compile_flags
    m_compile_file.pssl ?= $(OBJDIR)/pssl.compile_flags

    ifneq ($(COMPILE.cxx),)
        m_hash_compile.cxx  = $(shell sha256sum <<< "$$makeincr_COMPILE_cxx" | cut -c-64)
        ifeq ($(shell cmp -s $(m_compile_file.cxx) <(echo "$(m_hash_compile.cxx)") || echo 1),1)
            null := $(shell rm -f $(m_compile_file.cxx))
        endif
    endif

    ifneq ($(COMPILE.c),)
        m_hash_compile.c    = $(shell sha256sum <<< "$$makeincr_COMPILE_c" | cut -c-64)
        ifeq ($(shell cmp -s $(m_compile_file.c) <(echo "$(m_hash_compile.c)") || echo 1),1)
            null := $(shell rm -f $(m_compile_file.c))
        endif
    endif

    ifneq ($(COMPILE.pssl),)
        m_hash_compile.pssl  = $(shell sha256sum <<< "$$makeincr_COMPILE_pssl" | cut -c-64)
        ifeq ($(shell cmp -s $(m_compile_file.pssl) <(echo "$(m_hash_compile.pssl)") || echo 1),1)
            null := $(shell rm -f $(m_compile_file.pssl))
        endif
    endif
endif

mkobjdir = @[[ -d '$(@D)' ]] || mkdir -p '$(@D)'

m_old_prefix = $(.RECIPEPREFIX)
.RECIPEPREFIX = :

$(m_link_incr_file):
:   $(call mkobjdir)
:   @echo $(m_hash_linker) > $(m_link_incr_file)

$(m_compile_file.cxx):
:   $(call mkobjdir)
:   @echo $(m_hash_compile.cxx) > $(m_compile_file.cxx)

$(m_compile_file.c):
:   $(call mkobjdir)
:   @echo $(m_hash_compile.c) > $(m_compile_file.c)

$(m_compile_file.pssl):
:   $(call mkobjdir)
:   @echo $(m_hash_compile.pssl) > $(m_compile_file.pssl)

.RECIPEPREFIX = $(m_old_prefix)
