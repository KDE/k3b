AM_CPPFLAGS= 	-I$(srcdir)/../../core \
		-I$(top_srcdir)/libk3bdevice \
		-I$(srcdir)/../../tools \
		-I$(srcdir)/.. \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libvcd.la

libvcd_la_SOURCES = k3bvcddoc.cpp k3bvcdtrack.cpp k3bvcdjob.cpp k3bvcdview.cpp k3bvcdlistviewitem.cpp k3bvcdlistview.cpp k3bvcdburndialog.cpp k3bvcdoptions.cpp k3bvcdtrackdialog.cpp k3bvcdxmlview.cpp

libvcd_la_LIBADD = mpeginfo/libmpeginfo.la

SUBDIRS = mpeginfo cdi

include_HEADERS = k3bvcdjob.h k3bvcddoc.h k3bvcdview.h k3bvcdoptions.h
