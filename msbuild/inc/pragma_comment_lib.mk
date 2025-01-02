# Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

# usage:
#  1. include this file somewhere inside your rules block.
#    a. Including it too soon can cause problems, due to Makefile evaluation of rules dependencies at the point
#       where a rule is defined.
#  2. add $(PRAGMA_LIB_DEPS.LD) to your Linker Steps.
#
# Depends on Common Makefile vars:
#   CPPFLAGS
#   OBJDIR
#
# Known Limitations
#  - Only works well for makefiles that generate a single linked target.
#      (in general makefile has many limitations that make it not well suited to the task of building may different
#       complex build + link jobs that depend on vastly different configs)

# Default Behavior: assume all OBJECTS should be parsed for #pragma comment(lib,) according to c/cpp extension rules.
ifeq ($(PRAGMA_LIB_DEPS.LD),)
    PRAGMA_LIB_DEPS.LD := $(OBJECTS:.o=.pragma_lib)
endif

m_pragma_comment_mydir := $(dir $(realpath -m $(lastword $(MAKEFILE_LIST))))

$(TARGET_FULLPATH): LDFLAGS += $(shell cat $(PRAGMA_LIB_DEPS.LD) < /dev/null 2> /dev/null)
$(OBJDIR)/%.pragma_lib: %.cpp $(INCREMENTAL_DEPS.CXX)
:@  $(CXX) -E $< $(all_includes.macro) $(CPPFLAGS) | $(m_pragma_comment_mydir)/pragma_comment_lib_list.sh $@

$(OBJDIR)/%.pragma_lib: %.c $(INCREMENTAL_DEPS.C)
:@  $(CC) -E $< $(all_includes.macro) $(CPPFLAGS) | $(m_pragma_comment_mydir)/pragma_comment_lib_list.sh $@
