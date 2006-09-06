AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core \
	-I$(srcdir)/../../../libk3b/core \
	-I$(srcdir)/../../../libk3b/plugin \
	-I$(srcdir)/../../../libk3b/tools \
	-I$(srcdir)/../../../libk3b/projects \
	-I$(srcdir)/../../../libk3b/projects/datacd \
	-I$(srcdir)/../../../libk3bdevice \
	$(all_includes)

kde_module_LTLIBRARIES = libk3baudiometainforenamerplugin.la

libk3baudiometainforenamerplugin_la_SOURCES = k3baudiometainforenamerplugin.cpp
libk3baudiometainforenamerplugin_la_LIBADD = ../../../libk3b/libk3b.la
libk3baudiometainforenamerplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3baudiometainforenamerplugin.plugin

METASOURCES = AUTO
