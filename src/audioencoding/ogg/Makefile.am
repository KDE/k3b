AM_CPPFLAGS = -I$(srcdir)/../../plugin -I$(srcdir)/../../core -I$(srcdir)/../../core/device $(all_includes)

kde_module_LTLIBRARIES = libk3boggvorbisencoder.la

libk3boggvorbisencoder_la_SOURCES = base_k3boggvorbisencodersettingswidget.ui k3boggvorbisencoder.cpp

libk3boggvorbisencoder_la_LIBADD = $(LIB_KDECORE) -logg -lvorbis -lvorbisenc ../../plugin/libk3bplugin.la
# ../../core/libk3bcore.la
libk3boggvorbisencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3boggvorbisencoder.plugin

METASOURCES = AUTO
