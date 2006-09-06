AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin $(all_includes)

kde_module_LTLIBRARIES = libk3bartsoutputplugin.la

libk3bartsoutputplugin_la_SOURCES = k3bartsoutputplugin.cpp

libk3bartsoutputplugin_la_LIBADD = ../../../libk3b/libk3b.la -lartsc
libk3bartsoutputplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bartsoutputplugin.plugin

METASOURCES = AUTO
