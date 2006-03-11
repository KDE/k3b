AM_CPPFLAGS= -I$(srcdir)/../../../src \
	-I$(srcdir)/.. \
	-I$(srcdir)/../../core \
	-I$(srcdir)/../../plugin \
	-I$(srcdir)/../../cddb \
	-I$(srcdir)/../../../libk3bdevice \
	-I$(srcdir)/../../tools $(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libaudio.la

libaudio_la_SOURCES = 	k3baudiojob.cpp \
			k3baudiotrack.cpp \
			k3baudiodoc.cpp \
			k3baudiofile.cpp \
			k3baudiozerodata.cpp \
			k3baudiodatasource.cpp \
			k3baudionormalizejob.cpp \
			k3baudiojobtempdata.cpp \
			k3baudioimager.cpp \
			k3baudiomaxspeedjob.cpp \
			k3baudiocdtracksource.cpp \
			k3baudiocdtrackdrag.cpp \
			k3baudiodatasourceiterator.cpp

include_HEADERS = 	k3baudiodoc.h \
			k3baudiojob.h \
			k3baudiocdtrackdrag.h \
			k3baudiotrack.h \
			k3baudiodatasource.h \
			k3baudiofile.h \
			k3baudiozerodata.h \
			k3baudiocdtracksource.h \
			k3baudiodatasourceiterator.h
