AM_CPPFLAGS = -I$(srcdir)/../../../libk3b/core -I$(srcdir)/../../../libk3b/tools -I$(srcdir)/../../../libk3b/projects -I$(srcdir)/../../../libk3bdevice -I$(srcdir)/../../../libk3b/plugin $(all_includes)

lib_LTLIBRARIES = libk3baudioconverterplugin.la

libk3baudioconverterplugin_la_SOURCES = base_k3baudioconverterwidget.ui \
					k3baudioconverterplugin.cpp \
					k3baudioconverterviewitem.cpp \
					k3baudioconverterjob.cpp \
					k3baudioconverterview.cpp
libk3baudioconverterplugin_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KPARTS)
libk3baudioconverterplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/kpartplugins
plugins_DATA = k3baudioconverterplugin.rc

METASOURCES = AUTO
