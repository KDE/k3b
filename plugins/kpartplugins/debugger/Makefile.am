AM_CPPFLAGS =   -I$(srcdir)/../../../libk3b/core \
                -I$(srcdir)/../../../libk3b/tools \
                -I$(srcdir)/../../../libk3b/projects \
                -I$(srcdir)/../../../libk3bdevice \
                $(all_includes)

#AM_LDFLAGS = $(all_libraries)
#AM_LDADD = $(LIB_KPARTS) $(LIB_KDECORE)

lib_LTLIBRARIES = libk3bdebuggerplugin.la

libk3bdebuggerplugin_la_SOURCES = k3bdebuggerplugin.cpp
libk3bdebuggerplugin_la_LIBADD = ../../../libk3b/libk3b.la $(LIB_KPARTS)
libk3bdebuggerplugin_la_LDFLAGS = -avoid-version -module -no-undefined

pluginsdir = $(kde_datadir)/k3b/kpartplugins
plugins_DATA = k3bdebuggerplugin.rc

METASOURCES = AUTO
