AM_CPPFLAGS = -I$(top_srcdir)/libk3b/plugin -I$(top_srcdir)/libk3b/core -I$(top_srcdir)/libk3bdevice $(all_includes)

kde_module_LTLIBRARIES = libk3blameencoder.la

libk3blameencoder_la_SOURCES = base_k3blameencodersettingswidget.ui k3blameencoder.cpp

libk3blameencoder_la_LIBADD = $(top_builddir)/libk3b/libk3b.la $(LIB_KDECORE) -lmp3lame
libk3blameencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3blameencoder.plugin

METASOURCES = AUTO
