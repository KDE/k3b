AM_CPPFLAGS = -I$(srcdir)/../../libk3bdevice -I$(srcdir)/../plugin -I$(srcdir)/../tools $(all_includes)

noinst_LTLIBRARIES = libk3bcore.la

libk3bcore_la_LIBADD = $(LIB_KIO) $(ARTSC_LIB)

libk3bcore_la_LDFLAGS = $(all_libraries)

libk3bcore_la_SOURCES = k3bcore.cpp k3bglobals.cpp k3bdefaultexternalprograms.cpp \
	k3bexternalbinmanager.cpp k3bversion.cpp k3bprocess.cpp k3bjob.cpp \
	k3bthread.cpp k3bthreadjob.cpp k3bglobalsettings.cpp k3bsimplejobhandler.cpp

include_HEADERS = k3bcore.h k3bdefaultexternalprograms.h k3bexternalbinmanager.h \
	k3bprocess.h k3bversion.h k3bglobals.h k3bjob.h k3bthread.h \
	k3bthreadjob.h k3bglobalsettings.h k3bjobhandler.h \
	k3b_export.h k3bjobhandler.h k3bsimplejobhandler.h

METASOURCES = AUTO

