AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3bdevice -I$(srcdir)/../../../libk3b/core $(taglib_includes) $(all_includes)

kde_module_LTLIBRARIES = libk3bflacdecoder.la

libk3bflacdecoder_la_SOURCES = k3bflacdecoder.cpp

libk3bflacdecoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDEUI) -lFLAC++ -lFLAC $(taglib_libs) 
libk3bflacdecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bflacdecoder.plugin

METASOURCES = AUTO
