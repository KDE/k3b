AM_CPPFLAGS = -I$(srcdir)/../../plugin -I$(srcdir)/../../core -I$(srcdir)/../../device $(all_includes)

kde_module_LTLIBRARIES = libk3blameencoder.la

libk3blameencoder_la_SOURCES = base_k3blameencodersettingswidget.ui k3blameencoder.cpp

libk3blameencoder_la_LIBADD = $(LIB_KDECORE) -lmp3lame ../../plugin/libk3bplugin.la
libk3blameencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3blameencoder.plugin

METASOURCES = AUTO
