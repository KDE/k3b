bin_PROGRAMS = growisofs dvd+rw-format

growisofs_SOURCES = growisofs.c growisofs_mmc.cpp
dvd_rw_format_SOURCES = dvd+rw-format.cpp

install-exec-hook:
	@(chown 0 $(DESTDIR)$(bindir)/growisofs && chmod 4755 $(DESTDIR)$(bindir)/growisofs)