AM_CPPFLAGS = -I$(srcdir)/../../../core -I$(srcdir)/../../../ $(all_includes)
#AM_LDFLAGS = $(all_libraries)
#AM_LDADD = $(LIB_KPARTS) $(LIB_KDECORE)

lib_LTLIBRARIES = libk3bdebuggerplugin.la

libk3bdebuggerplugin_la_SOURCES = k3bdebuggerplugin.cpp
libk3bdebuggerplugin_la_LIBADD = $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KPARTS) ../../../core/libk3bcore.la
libk3bdebuggerplugin_la_LDFLAGS = -avoid-version -module -no-undefined

pluginsdir = $(kde_datadir)/k3b/kpartplugins
plugins_DATA = k3bdebuggerplugin.rc

METASOURCES = AUTO
