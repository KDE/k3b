AM_CPPFLAGS = -I$(top_srcdir)/libk3b/core -I$(top_srcdir)/libk3b/tools -I$(top_srcdir)/libk3b/projects -I$(top_srcdir)/libk3bdevice -I$(top_srcdir)/libk3b/plugin $(all_includes)

lib_LTLIBRARIES = libk3baudioconverterplugin.la

libk3baudioconverterplugin_la_SOURCES = base_k3baudioconverterwidget.ui \
					k3baudioconverterplugin.cpp \
					k3baudioconverterviewitem.cpp \
					k3baudioconverterjob.cpp \
					k3baudioconverterview.cpp
libk3baudioconverterplugin_la_LIBADD = $(top_builddir)/libk3b/libk3b.la $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KPARTS)
libk3baudioconverterplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/kpartplugins
plugins_DATA = k3baudioconverterplugin.rc

METASOURCES = AUTO
