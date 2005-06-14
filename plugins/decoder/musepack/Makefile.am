AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/plugin \
	-I$(srcdir)/../../../libk3b/core \
	-I$(srcdir)/../../../libk3bdevice \
	$(all_includes)

kde_module_LTLIBRARIES = libk3bmpcdecoder.la

libk3bmpcdecoder_la_SOURCES = k3bmpcdecoder.cpp k3bmpcwrapper.cpp

libk3bmpcdecoder_la_LIBADD = ../../../libk3b/libk3b.la $(MPC_LIBS)
libk3bmpcdecoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bmpcdecoder.plugin

METASOURCES = AUTO
