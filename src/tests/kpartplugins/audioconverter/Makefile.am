AM_CPPFLAGS = -I$(srcdir)/../../../core -I$(srcdir)/../../../tools -I$(srcdir)/../../../projects -I$(srcdir)/../../../projects/datacd -I$(srcdir)/../../../device -I$(srcdir)/../../../plugin $(all_includes)

lib_LTLIBRARIES = libk3baudioconverterplugin.la

libk3baudioconverterplugin_la_SOURCES = base_k3baudioconverterwidget.ui \
					k3baudioconverterplugin.cpp \
					k3baudioconverterviewitem.cpp \
					k3baudioconverterjob.cpp \
					k3baudioconverterview.cpp
libk3baudioconverterplugin_la_LIBADD = $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KPARTS) ../../../core/libk3bcore.la ../../../tools/libk3btools.la  ../../../plugin/libk3bplugin.la  ../../../projects/libk3bproject.la
libk3baudioconverterplugin_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/k3b/kpartplugins
plugins_DATA = k3baudioconverterplugin.rc

METASOURCES = AUTO
