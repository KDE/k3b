AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/plugin $(all_includes)

kde_module_LTLIBRARIES = libk3bartsaudioserver.la

libk3bartsaudioserver_la_SOURCES = k3bartsaudioserver.cpp

libk3bartsaudioserver_la_LIBADD = ../../../libk3b/libk3b.la -lartsc
libk3bartsaudioserver_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bartsaudioserver.plugin

METASOURCES = AUTO
