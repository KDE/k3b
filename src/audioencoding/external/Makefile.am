AM_CPPFLAGS = -I$(srcdir)/../../plugin -I$(srcdir)/../../core -I$(srcdir)/../../core/device $(all_includes)

kde_module_LTLIBRARIES = libk3bexternalencoder.la

libk3bexternalencoder_la_SOURCES = base_k3bexternalencoderconfigwidget.ui k3bexternalencoder.cpp

libk3bexternalencoder_la_LIBADD = $(LIB_KDECORE) ../../plugin/libk3bplugin.la
# ../../core/libk3bcore.la
libk3bexternalencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bexternalencoder.plugin

METASOURCES = AUTO
