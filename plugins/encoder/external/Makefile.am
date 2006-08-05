AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)

kde_module_LTLIBRARIES = libk3bexternalencoder.la

libk3bexternalencoder_la_SOURCES = base_k3bexternalencoderconfigwidget.ui \
	base_k3bexternalencodereditwidget.ui k3bexternalencoder.cpp \
	k3bexternalencoderconfigwidget.cpp k3bexternalencodercommand.cpp

libk3bexternalencoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDECORE)
libk3bexternalencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3bexternalencoder.plugin

METASOURCES = AUTO
