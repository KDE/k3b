AM_CPPFLAGS = -I$(srcdir)/../../plugin -I$(srcdir)/../../device $(all_includes)

kde_module_LTLIBRARIES = libk3bmaddecoder.la

libk3bmaddecoder_la_SOURCES = k3bmaddecoder.cpp

libk3bmaddecoder_la_LIBADD = $(LIB_KDECORE) $(MAD_LIB) $(ID3_LIB) ../../plugin/libk3bplugin.la
# ../../core/libk3bcore.la
libk3bmaddecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bmaddecoder.plugin

METASOURCES = AUTO
