AM_CPPFLAGS = -I$(srcdir)/../../plugin -I$(srcdir)/../../core -I$(srcdir)/../../device $(all_includes)

kde_module_LTLIBRARIES = libk3bsoxencoder.la

libk3bsoxencoder_la_SOURCES = base_k3bsoxencoderconfigwidget.ui k3bsoxencoder.cpp

libk3bsoxencoder_la_LIBADD = $(LIB_KDECORE) ../../plugin/libk3bplugin.la
# ../../core/libk3bcore.la
libk3bsoxencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bsoxencoder.plugin

METASOURCES = AUTO
