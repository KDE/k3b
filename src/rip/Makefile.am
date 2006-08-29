if include_videodvdrip
VIDEODVDRIPDIR = videodvd
VIDEODVDRIPLIB = videodvd/libvideodvdrip.la
endif

AM_CPPFLAGS = 	-I$(srcdir)/../../libk3b/core \
		-I$(srcdir)/../../libk3b/cddb \
		-I$(srcdir)/../../libk3bdevice \
		-I$(srcdir)/../../libk3b/plugin \
		-I$(srcdir)/../../libk3b/tools \
		-I$(srcdir)/../../libk3b/projects \
		-I$(srcdir)/../../libk3b/projects/audiocd \
		-I$(srcdir)/../../libk3b/cddb \
		-I$(srcdir)/../../libk3b/jobs/ \
		-I$(srcdir)/.. \
		-I$(srcdir)/../projects \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = librip.la
librip_la_LIBADD = $(VIDEODVDRIPLIB)

librip_la_SOURCES = base_k3baudiorippingoptionwidget.ui \
	base_k3bcddbpatternwidget.ui k3bpatternparser.cpp k3baudiorippingdialog.cpp \
	k3baudioripthread.cpp k3baudiocdview.cpp k3bcddbpatternwidget.cpp \
	k3bvideocdinfo.cpp k3bvideocdview.cpp k3bvideocdrip.cpp \
	k3bvideocdrippingdialog.cpp k3bcuefilewriter.cpp k3baudioconvertingoptionwidget.cpp \
	k3baudiocdlistview.cpp k3baudioprojectconvertingdialog.cpp \
	k3baudioprojectconvertingthread.cpp k3baudioripjob.cpp

SUBDIRS = $(VIDEODVDRIPDIR)