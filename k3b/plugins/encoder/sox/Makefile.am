AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)

kde_module_LTLIBRARIES = libk3bsoxencoder.la

libk3bsoxencoder_la_SOURCES = base_k3bsoxencoderconfigwidget.ui k3bsoxencoder.cpp

libk3bsoxencoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDECORE)
libk3bsoxencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bsoxencoder.plugin

METASOURCES = AUTO
