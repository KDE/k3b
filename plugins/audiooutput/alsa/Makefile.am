AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin $(all_includes)

kde_module_LTLIBRARIES = libk3balsaoutputplugin.la

libk3balsaoutputplugin_la_SOURCES = k3balsaoutputplugin.cpp

libk3balsaoutputplugin_la_CFLAGS = $(ALSA_CFLAGS)
libk3balsaoutputplugin_la_LIBADD = ../../../libk3b/libk3b.la $(ALSA_LIBS)
libk3balsaoutputplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3balsaoutputplugin.plugin

METASOURCES = AUTO
