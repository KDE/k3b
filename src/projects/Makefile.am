AM_CPPFLAGS= -I$(srcdir)/../core -I$(srcdir)/../device -I$(srcdir)/../tools -I$(srcdir)/datacd -I$(srcdir)/audiocd -I$(srcdir)/mixedcd -I$(srcdir)/movixcd -I$(srcdir)/movixdvd -I$(srcdir)/datadvd -I$(srcdir)/videocd -I$(srcdir)/videodvd $(all_includes)

lib_LTLIBRARIES = libk3bproject.la

libk3bproject_la_LIBADD = ../device/libk3bdevice.la ../core/libk3bcore.la ../tools/libk3btools.la ./datacd/libdata.la ./datadvd/libdvd.la ./mixedcd/libmixed.la ./videocd/libvcd.la ./videodvd/libvideodvd.la ./movixcd/libmovix.la ./movixdvd/libmovixdvd.la ./audiocd/libaudio.la ./songdb/libsongdb.la ./kostore/libkostore.la ../plugin/libk3bplugin.la

libk3bproject_la_LDFLAGS = $(all_libraries) -version-info 2:0:0 -no-undefined

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
			   k3bdiskerasinginfodialog.cpp \
			   k3bprojectmanager.cpp \
			   k3binffilewriter.cpp

include_HEADERS =          k3bview.h \
                           k3bblankingjob.h \
                           k3bdoc.h \
	                   k3bwriterselectionwidget.h \
                           k3bemptydiscwaiter.h \
			   k3bprojectmanager.h

SUBDIRS = kostore songdb datacd audiocd mixedcd movixcd videocd datadvd movixdvd videodvd

METASOURCES = AUTO
