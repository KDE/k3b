AM_CPPFLAGS = -I$(top_srcdir)/libk3b/plugin -I$(top_srcdir)/libk3bdevice $(all_includes)

kde_module_LTLIBRARIES = libk3boggvorbisdecoder.la

libk3boggvorbisdecoder_la_SOURCES = k3boggvorbisdecoder.cpp

libk3boggvorbisdecoder_la_LIBADD = $(top_builddir)/libk3b/libk3b.la $(LIB_KDECORE) -logg -lvorbis -lvorbisfile $(LIB_KDEUI)
libk3boggvorbisdecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3boggvorbisdecoder.plugin

METASOURCES = AUTO
