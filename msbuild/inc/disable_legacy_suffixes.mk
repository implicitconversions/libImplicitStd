# Cancel these implicit rules which are throwbacks to CVS hacks in original versions of make.
.SUFFIXES:

%: %,v
%: RCS/%,v
%: RCS/%
%: s.%
%: SCCS/s.%
