
# Default Behavior: assume all OBJECTS should be parsed for #pragma comment(lib,) according to c/cpp extension rules.
ifeq ($(LINK_SWITCH_DEPS.LD),)
    LINK_SWITCH_DEPS.LD := $(OBJECTS:.o=.link_switch)
endif

m_pragma_comment_mydir := $(dir $(realpath -m $(lastword $(MAKEFILE_LIST))))

$(TARGET_FULLPATH): LDFLAGS += $(shell cat $(LINK_SWITCH_DEPS.LD) < /dev/null 2> /dev/null)
$(OBJDIR)/%.link_switch: %.cpp $(INCREMENTAL_DEPS.CXX)
:@  $(call mkobjdir)
:@  $(CXX) -E $< $(all_includes.macro) $(CPPFLAGS) | $(m_pragma_comment_mydir)/pragma_comment_lib_list.sh $@

$(OBJDIR)/%.link_switch: %.c $(INCREMENTAL_DEPS.C)
:@  $(call mkobjdir)
:@  $(CC) -E $< $(all_includes.macro) $(CPPFLAGS) | $(m_pragma_comment_mydir)/pragma_comment_lib_list.sh $@
