AM_CPPFLAGS = 	-I$(srcdir)/../../../libk3b/core \
		-I$(srcdir)/../../../libk3b/tools \
		-I$(srcdir)/../../../libk3b/core \
		-I$(srcdir)/../../../libk3b/projects/audiocd \
		-I$(srcdir)/../../../libk3b/projects \
		-I$(srcdir)/../../../libk3bdevice \
		-I$(srcdir)/../../../libk3b/cddb \
		-I$(srcdir)/../../../libk3b/plugin \
		$(all_includes)

kde_module_LTLIBRARIES = libk3baudioprojectcddbplugin.la

libk3baudioprojectcddbplugin_la_SOURCES = k3baudioprojectcddbplugin.cpp

libk3baudioprojectcddbplugin_la_LIBADD = ../../../libk3b/libk3b.la

libk3baudioprojectcddbplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3baudioprojectcddbplugin.plugin

METASOURCES = AUTO
