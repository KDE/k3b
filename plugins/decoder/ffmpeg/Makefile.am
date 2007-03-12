AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3bdevice $(all_includes)
KDE_CXXFLAGS = -D__STDC_CONSTANT_MACROS

kde_module_LTLIBRARIES = libk3bffmpegdecoder.la

libk3bffmpegdecoder_la_SOURCES = k3bffmpegdecoder.cpp k3bffmpegwrapper.cpp

libk3bffmpegdecoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDEUI) -lavcodec -lavformat
libk3bffmpegdecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bffmpegdecoder.plugin

METASOURCES = AUTO

