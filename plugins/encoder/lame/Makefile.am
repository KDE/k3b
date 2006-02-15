AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/plugin -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3bdevice $(all_includes)

kde_module_LTLIBRARIES = libk3blameencoder.la

libk3blameencoder_la_SOURCES = base_k3blameencodersettingswidget.ui \
	base_k3bmanualbitratesettingswidget.ui \
	k3blameencoder.cpp

libk3blameencoder_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KDECORE) -lmp3lame
libk3blameencoder_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/plugins
plugins_DATA = k3blameencoder.plugin

METASOURCES = AUTO
