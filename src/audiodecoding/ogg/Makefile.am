AM_CPPFLAGS = -I$(srcdir)/../../plugin -I$(srcdir)/../../core/device $(all_includes)

lib_LTLIBRARIES = libk3boggvorbisdecoder.la

libk3boggvorbisdecoder_la_SOURCES = k3boggvorbisdecoder.cpp

libk3boggvorbisdecoder_la_LIBADD = $(LIB_KDECORE) -logg -lvorbis -lvorbisfile ../../plugin/libk3bplugin.la $(LIB_KDEUI)
# ../../core/libk3bcore.la
libk3boggvorbisdecoder_la_LDFLAGS = -avoid-version -module -no-undefined

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3boggvorbisdecoder.plugin

METASOURCES = AUTO
