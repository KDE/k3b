AM_CPPFLAGS = 	-I$(srcdir)/../core \
		-I$(srcdir)/../../libk3bdevice \
		-I$(srcdir)/../tools \
		-I$(srcdir)/../plugin \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libvideodvd.la
libvideodvd_la_LIBADD = -ldvdread

libvideodvd_la_SOURCES = k3bvideodvd.cpp k3bvideodvdtime.cpp

include_HEADERS = k3bvideodvd.h k3bvideodvdtitle.h \
	k3bvideodvdaudiostream.h k3bvideodvdsubpicturestream.h \
	k3bvideodvdvideostream.h k3bvideodvdtime.h
