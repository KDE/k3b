AM_CPPFLAGS= 	-I$(srcdir)/../../core \
				-I$(srcdir)/../../../src \
		-I$(srcdir)/../../../libk3bdevice \
		-I$(srcdir)/../../tools \
		-I$(srcdir)/.. \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libvcd.la

libvcd_la_SOURCES = k3bvcddoc.cpp k3bvcdtrack.cpp k3bvcdjob.cpp k3bvcdoptions.cpp k3bvcdxmlview.cpp

libvcd_la_LIBADD = mpeginfo/libmpeginfo.la

SUBDIRS = cdi extra mpeginfo

include_HEADERS = 	k3bvcdjob.h \
			k3bvcddoc.h \
			k3bvcdoptions.h
