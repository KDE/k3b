lib_LTLIBRARIES = libk3b.la

libk3b_la_SOURCES = dummy.c

libk3b_la_LIBADD = 	core/libk3bcore.la \
			cddb/libcddb.la \
			projects/libk3bproject.la \
			plugin/libk3bplugin.la \
			tools/libk3btools.la \
			$(top_builddir)/libk3bdevice/libk3bdevice.la \
			-lartsc

libk3b_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

SUBDIRS = core plugin tools projects cddb

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui"` > rc.cpp
	$(XGETTEXT) --from-code=UTF-8 `find -name "*.cpp" -or -name "*.h"` -o $(podir)/libk3b.pot
