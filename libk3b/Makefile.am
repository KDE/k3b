lib_LTLIBRARIES = libk3b.la

libk3b_la_SOURCES = dummy.cpp

libk3b_la_LIBADD = 	core/libk3bcore.la \
			cddb/libcddb.la \
			projects/libk3bproject.la \
			plugin/libk3bplugin.la \
			tools/libk3btools.la \
			jobs/libjobs.la \
			../libk3bdevice/libk3bdevice.la

libk3b_la_LDFLAGS = $(all_libraries) -version-info 2:0:0 -no-undefined

SUBDIRS = core plugin tools projects cddb jobs 

#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = libk3b.pc

messages:
	$(XGETTEXT) `find -name "*.cpp" -o -name "*.h"` -o $(podir)/libk3b.pot
