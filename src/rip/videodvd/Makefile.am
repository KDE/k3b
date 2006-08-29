AM_CPPFLAGS = 	-I$(srcdir)/../../../libk3b/core \
		-I$(srcdir)/../../../libk3bdevice \
		-I$(srcdir)/../../../libk3b/tools \
		-I$(srcdir)/../../../libk3b/videodvd/ \
		-I$(srcdir)/../../../libk3b/jobs/ \
		-I$(srcdir)/../.. \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libvideodvdrip.la

libvideodvdrip_la_SOURCES = base_k3bvideodvdrippingwidget.ui k3bvideodvdrippingview.cpp \
	k3bvideodvdrippingtitlelistview.cpp \
	k3bvideodvdrippingjob.cpp k3bvideodvdrippingwidget.cpp \
	k3bvideodvdrippingdialog.cpp k3bvideodvdrippingpreview.cpp
