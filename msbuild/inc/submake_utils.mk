# Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

# helper to eliminate redundant switches in parameters handed to us by parent makefile.
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

# usage: $(call submake,library_dir,target,relative_dir_fixup)
#  $1 - library_dir is the directory of the library to submake-invoke.
#  $2 - target is the target name to build (empty is ok if the makefile has suitable default)
#  $3 - relative_dir_fixup is some number of ../.. needed to get from the library dir back to the
#       root dir of the caller makefile (this one)
#  $4 - Any extra arguments, if needed.
submake  = $(MAKE) INCLUDE_FLAGS="$(shell ./msbuild/submake_relative_includes.sh $1 $(INCLUDE_FLAGS))"
submake +=   --include-dir="$(abspath $(LIB_IMPLICIT_STD_DIR)/msbuild/inc)"
submake +=   -I$3/msbuild OBJDIR="$3/$(OBJDIR)/$1" $4 -C $1 $2

ifeq ($(INCREMENTAL),1)
    m_force_submake := FORCE
else
    m_force_submake := 
endif
