AM_CPPFLAGS = -I$(srcdir)/../../plugin -I$(srcdir)/../../device $(all_includes)

kde_module_LTLIBRARIES = libk3bflacdecoder.la

libk3bflacdecoder_la_SOURCES = k3bflacdecoder.cpp

libk3bflacdecoder_la_LIBADD = $(LIB_KDECORE) -lFLAC++ -lFLAC $(ID3_LIB) ../../plugin/libk3bplugin.la $(LIB_KDEUI)
# ../../core/libk3bcore.la
libk3bflacdecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bflacdecoder.plugin

METASOURCES = AUTO
