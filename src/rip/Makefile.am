AM_CPPFLAGS= -I$(srcdir)/../core -I$(srcdir)/.. $(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = librip.la


librip_la_SOURCES = k3bdvdriplistviewitem.cpp k3bmovieview.cpp k3bdvdextraripstatus.cpp k3bdvdfilldisplay.cpp k3bdvdaudiogain.cpp k3bdvdcopy.cpp k3bdvdrippingprocess.cpp k3bdvdripperwidget.cpp k3bdvdcontent.cpp k3btcwrapper.cpp k3bpatternparser.cpp k3bcddacopy.cpp k3baudiorippingdialog.cpp k3baudioripthread.cpp k3baudiocdview.cpp

librip_la_LIBADD = ../cddb/libcddb.la 
#AM_LDFLAGS = dvdread
SUBDIRS = songdb 

