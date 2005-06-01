AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3bdevice $(all_includes)

kde_module_LTLIBRARIES = libk3blibsndfiledecoder.la

libk3blibsndfiledecoder_la_SOURCES = k3blibsndfiledecoder.cpp

libk3blibsndfiledecoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDEUI) -lsndfile
libk3blibsndfiledecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3blibsndfiledecoder.plugin

METASOURCES = AUTO

