AM_CPPFLAGS = -I$(top_srcdir)/libk3b/plugin -I$(top_srcdir)/libk3bdevice $(all_includes)

kde_module_LTLIBRARIES = libk3bmaddecoder.la

libk3bmaddecoder_la_SOURCES = k3bmad.cpp k3bmaddecoder.cpp

libk3bmaddecoder_la_LIBADD = $(LIB_KDECORE) $(MAD_LIB) $(ID3_LIB) $(top_builddir)/libk3b/libk3b.la
libk3bmaddecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bmaddecoder.plugin

METASOURCES = AUTO
