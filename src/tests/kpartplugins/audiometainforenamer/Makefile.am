AM_CPPFLAGS = -I$(srcdir)/../../../core -I$(srcdir)/../../../tools -I$(srcdir)/../../../projects -I$(srcdir)/../../../projects/datacd $(all_includes)

lib_LTLIBRARIES = libk3baudiometainforenamerplugin.la

libk3baudiometainforenamerplugin_la_SOURCES = k3baudiometainforenamerplugin.cpp
libk3baudiometainforenamerplugin_la_LIBADD = $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KPARTS) ../../../core/libk3bcore.la ../../../tools/libk3btools.la  ../../../projects/libk3bproject.la 
libk3baudiometainforenamerplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/kpartplugins
plugins_DATA = k3baudiometainforenamerplugin.rc

METASOURCES = AUTO
