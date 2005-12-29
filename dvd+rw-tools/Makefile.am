bin_PROGRAMS = growisofs dvd+rw-format

growisofs_SOURCES = growisofs.c growisofs_mmc.cpp
growisofs_LDADD = $(CAM_LIB)

dvd_rw_format_SOURCES = dvd+rw-format.cpp
dvd_rw_format_LDADD = $(CAM_LIB)

install-exec-hook:
	@(chown 0 $(DESTDIR)$(bindir)/growisofs && chmod 4755 $(DESTDIR)$(bindir)/growisofs) || echo "Please make growisofs setuid root" >&2
	@echo ""
	@echo "growisofs is by default installed with a set SETUID root bit!"
	@echo ""

KDE_CXXFLAGS= -U__STRICT_ANSI__

