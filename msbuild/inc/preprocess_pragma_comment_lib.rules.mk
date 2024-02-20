
# Default Behavior: assume all OBJECTS should be parsed for #pragma comment(lib,) according to c/cpp extension rules.
ifeq ($(PRAGMA_LIB_DEPS.LD),)
    PRAGMA_LIB_DEPS.LD := $(OBJECTS:.o=.pragma_lib)
endif

m_pragma_comment_mydir := $(dir $(realpath -m $(lastword $(MAKEFILE_LIST))))

$(TARGET_FULLPATH): LDFLAGS += $(shell cat $(PRAGMA_LIB_DEPS.LD) < /dev/null 2> /dev/null)
$(OBJDIR)/%.pragma_lib: %.cpp $(INCREMENTAL_DEPS.CXX)
:@  $(call mkobjdir)
:@  $(CXX) -E $< $(all_includes.macro) $(CPPFLAGS) | $(m_pragma_comment_mydir)/pragma_comment_lib_list.sh $@

$(OBJDIR)/%.pragma_lib: %.c $(INCREMENTAL_DEPS.C)
:@  $(call mkobjdir)
:@  $(CC) -E $< $(all_includes.macro) $(CPPFLAGS) | $(m_pragma_comment_mydir)/pragma_comment_lib_list.sh $@
