AM_CPPFLAGS= -I$(srcdir)/../core -I$(srcdir)/../tools -I$(srcdir)/datacd -I$(srcdir)/audiocd -I$(srcdir)/mixedcd -I$(srcdir)/movixcd -I$(srcdir)/datadvd -I$(srcdir)/videocd $(all_includes)

lib_LTLIBRARIES = libk3bproject.la

libk3bproject_la_LIBADD = ../core/libk3bcore.la ../tools/libk3btools.la ./datacd/libdata.la ./datadvd/libdvd.la ./mixedcd/libmixed.la ./videocd/libvcd.la ./movixcd/libmovix.la ./audiocd/libaudio.la ./songdb/libsongdb.la ./kostore/libkostore.la

# FIXME: improve that versioning stuff (info libtool -> versioning)
libk3bproject_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libk3bproject_la_SOURCES = k3babstractwriter.cpp \
	                   k3bdatamodewidget.cpp \
	                   k3bgrowisofswriter.cpp \
	                   k3bview.cpp \
                           k3bblankingjob.cpp \
                           k3bdoc.cpp \
	                   k3bwriterselectionwidget.cpp \
	                   k3bdvdrecordwriter.cpp \
                           k3bwritingmodewidget.cpp \
                           k3bcdrdaowriter.cpp \
                           k3bemptydiscwaiter.cpp \
                           k3bprojectburndialog.cpp \
                           k3bcdrecordwriter.cpp \
                           k3bfillstatusdisplay.cpp \
                           k3btempdirselectionwidget.cpp \
			   k3bdiskerasinginfodialog.cpp

include_HEADERS =          k3bview.h \
                           k3bblankingjob.h \
                           k3bdoc.h \
	                   k3bwriterselectionwidget.h \
                           k3bemptydiscwaiter.h

SUBDIRS = kostore songdb datacd audiocd mixedcd movixcd videocd datadvd

METASOURCES = AUTO
