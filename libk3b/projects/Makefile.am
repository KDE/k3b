AM_CPPFLAGS= -I$(srcdir)/../core -I$(top_srcdir)/libk3bdevice -I$(srcdir)/../tools -I$(srcdir)/datacd -I$(srcdir)/audiocd -I$(srcdir)/mixedcd -I$(srcdir)/movixcd -I$(srcdir)/movixdvd -I$(srcdir)/datadvd -I$(srcdir)/videocd -I$(srcdir)/videodvd $(all_includes)

noinst_LTLIBRARIES = libk3bproject.la

libk3bproject_la_LIBADD = ./datacd/libdata.la ./datadvd/libdvd.la ./mixedcd/libmixed.la ./videocd/libvcd.la ./videodvd/libvideodvd.la ./movixcd/libmovix.la ./movixdvd/libmovixdvd.la ./audiocd/libaudio.la ./songdb/libsongdb.la ./kostore/libkostore.la

libk3bproject_la_LDFLAGS = $(all_libraries)

libk3bproject_la_SOURCES = k3babstractwriter.cpp \
	                   k3bdatamodewidget.cpp \
	                   k3bgrowisofswriter.cpp \
	                   k3bgrowisofshandler.cpp \
	                   k3bview.cpp \
                           k3bblankingjob.cpp \
                           k3bdoc.cpp \
	                   k3bwriterselectionwidget.cpp \
                           k3bwritingmodewidget.cpp \
                           k3bcdrdaowriter.cpp \
                           k3bemptydiscwaiter.cpp \
                           k3bprojectburndialog.cpp \
                           k3bcdrecordwriter.cpp \
                           k3bfillstatusdisplay.cpp \
                           k3btempdirselectionwidget.cpp \
			   k3bprojectmanager.cpp \
			   k3binffilewriter.cpp \
			   k3btocfilewriter.cpp \
			   k3bjobprogressdialog.cpp \
			   k3bburnprogressdialog.cpp \
			   k3bjobprogresssystemtray.cpp \
			   k3bimagefilereader.cpp \
			   k3bcuefileparser.cpp \
			   k3bpipebuffer.cpp

include_HEADERS =          k3bview.h \
                           k3bblankingjob.h \
                           k3bdoc.h \
	                   k3bwriterselectionwidget.h \
                           k3bemptydiscwaiter.h \
			   k3bprojectmanager.h \
			   k3bjobprogressdialog.h \
			   k3bburnprogressdialog.h \
			   k3bpipebuffer.h

SUBDIRS = kostore songdb datacd audiocd mixedcd movixcd videocd datadvd movixdvd videodvd

METASOURCES = AUTO
