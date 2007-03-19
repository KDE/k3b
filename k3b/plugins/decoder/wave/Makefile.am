AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)

kde_module_LTLIBRARIES = libk3bwavedecoder.la

libk3bwavedecoder_la_SOURCES = k3bwavedecoder.cpp

libk3bwavedecoder_la_LIBADD = $(LIB_KDECORE) ../../../libk3b/libk3b.la
libk3bwavedecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bwavedecoder.plugin

METASOURCES = AUTO
