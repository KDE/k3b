AM_CPPFLAGS = -I$(srcdir)/../../../core -I$(srcdir)/../../../tools -I$(srcdir)/../../../projects/audiocd -I$(srcdir)/../../../device -I$(srcdir)/../../../cddb $(all_includes)

lib_LTLIBRARIES = libk3baudioprojectcddbplugin.la

libk3baudioprojectcddbplugin_la_SOURCES = k3baudioprojectcddbplugin.cpp

libk3baudioprojectcddbplugin_la_LIBADD = $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KPARTS) ../../../core/libk3bcore.la ../../../tools/libk3btools.la  ../../../cddb/libcddb.la  ../../../projects/libk3bproject.la

libk3baudioprojectcddbplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/kpartplugins
plugins_DATA = k3baudioprojectcddbplugin.rc

METASOURCES = AUTO
