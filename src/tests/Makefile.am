AM_CPPFLAGS = $(all_includes) -I../core -I..
LDFLAGS	= $(all_libraries)
LDADD = $(LIB_KPARTS) $(LIB_KDECORE)

lib_LTLIBRARIES = libk3bdummyplugin.la

libk3bdummyplugin_la_SOURCES = k3bdummyplugin.cpp
libk3bdummyplugin_la_LIBADD = $(LIB_KDEUI) $(LIB_KPARTS) ../core/libk3bcore.la
libk3bdummyplugin_la_LDFLAGS = -avoid-version -module -no-undefined

pluginsdir = $(kde_datadir)/k3b/kpartplugins
plugins_DATA = k3bdummyplugin.rc

METASOURCES = AUTO
