AM_CPPFLAGS = -I$(srcdir)/device $(all_includes)

lib_LTLIBRARIES = libk3bcore.la

libk3bcore_la_LIBADD = device/libdevice.la $(LIB_KIO) $(RESMGR_LIB)

# FIXME: improve that versioning stuff (info libtool -> versioning)
libk3bcore_la_LDFLAGS = $(all_libraries) -version-info 2:0:0 -no-undefined

libk3bcore_la_SOURCES = k3bcore.cpp k3bglobals.cpp k3bdefaultexternalprograms.cpp k3bexternalbinmanager.cpp k3bversion.cpp k3bexternalbinwidget.cpp k3bprocess.cpp k3blistview.cpp k3bmsfedit.cpp k3bjob.cpp k3bthread.cpp k3bthreadjob.cpp

include_HEADERS = k3bcore.h k3bdefaultexternalprograms.h k3bexternalbinmanager.h k3bexternalbinwidget.h k3bprocess.h k3bversion.h k3bglobals.h k3blistview.h k3bmsfedit.h k3bjob.h k3bthread.h k3bthreadjob.h

METASOURCES = AUTO

SUBDIRS = device
