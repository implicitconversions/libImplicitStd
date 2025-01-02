# Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

# Cancel these implicit rules which are throwbacks to CVS hacks in original versions of make.
.SUFFIXES:

%: %,v
%: RCS/%,v
%: RCS/%
%: s.%
%: SCCS/s.%
