AM_CPPFLAGS = -I$(top_srcdir)/libk3b/core -I$(top_srcdir)/libk3b/tools -I$(top_srcdir)/libk3b/projects -I$(top_srcdir)/libk3b/projects/datacd $(all_includes)

lib_LTLIBRARIES = libk3baudiometainforenamerplugin.la

libk3baudiometainforenamerplugin_la_SOURCES = k3baudiometainforenamerplugin.cpp
libk3baudiometainforenamerplugin_la_LIBADD = $(top_builddir)/libk3b/libk3b.la $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KPARTS)
libk3baudiometainforenamerplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/kpartplugins
plugins_DATA = k3baudiometainforenamerplugin.rc

METASOURCES = AUTO
