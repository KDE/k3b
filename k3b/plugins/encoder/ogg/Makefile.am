AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)

kde_module_LTLIBRARIES = libk3boggvorbisencoder.la

libk3boggvorbisencoder_la_SOURCES = base_k3boggvorbisencodersettingswidget.ui k3boggvorbisencoder.cpp

libk3boggvorbisencoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDECORE) -logg -lvorbis -lvorbisenc
libk3boggvorbisencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3boggvorbisencoder.plugin

METASOURCES = AUTO
