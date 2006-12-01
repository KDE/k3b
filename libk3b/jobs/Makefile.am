AM_CPPFLAGS = 	-I$(srcdir)/../core \
		-I$(srcdir)/../../libk3bdevice \
		-I$(srcdir)/../../src \
		-I$(srcdir)/../tools \
		-I$(srcdir)/../cddb \
		-I$(srcdir)/../plugin \
		-I$(srcdir)/../projects \
		-I$(srcdir)/../videodvd \
		-I$(srcdir)/../projects/audiocd \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libjobs.la

if include_videodvdrip
libjobs_la_SOURCES = k3bdatatrackreader.cpp k3breadcdreader.cpp \
	k3bcdcopyjob.cpp k3bclonejob.cpp k3baudiosessionreadingjob.cpp \
	k3bdvdcopyjob.cpp k3bvideodvdtitletranscodingjob.cpp k3bvideodvdtitledetectclippingjob.cpp \
	k3baudiocuefilewritingjob.cpp k3bbinimagewritingjob.cpp \
	k3biso9660imagewritingjob.cpp \
	k3bdvdformattingjob.cpp k3bblankingjob.cpp k3bclonetocreader.cpp \
	k3bverificationjob.cpp

include_HEADERS = k3bcdcopyjob.h k3bdvdcopyjob.h k3bclonejob.h \
	k3baudiocuefilewritingjob.h k3bbinimagewritingjob.h \
	k3biso9660imagewritingjob.h k3bdvdformattingjob.h \
	k3bblankingjob.h k3bvideodvdtitletranscodingjob.h k3bvideodvdtitledetectclippingjob.h \
	k3bverificationjob.h
else
libjobs_la_SOURCES = k3bdatatrackreader.cpp k3breadcdreader.cpp \
	k3bcdcopyjob.cpp k3bclonejob.cpp k3baudiosessionreadingjob.cpp \
	k3bdvdcopyjob.cpp \
	k3baudiocuefilewritingjob.cpp k3bbinimagewritingjob.cpp \
	k3biso9660imagewritingjob.cpp \
	k3bdvdformattingjob.cpp k3bblankingjob.cpp k3bclonetocreader.cpp \
	k3bverificationjob.cpp

include_HEADERS = k3bcdcopyjob.h k3bdvdcopyjob.h k3bclonejob.h \
	k3baudiocuefilewritingjob.h k3bbinimagewritingjob.h \
	k3biso9660imagewritingjob.h k3bdvdformattingjob.h \
	k3bblankingjob.h k3bverificationjob.h
endif
