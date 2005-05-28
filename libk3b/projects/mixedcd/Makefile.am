# we need the ../datacd and ../audiocd for the uic generated header files
AM_CPPFLAGS=	-I$(srcdir)/../../core \
		-I$(srcdir)/../../plugin \
		-I$(srcdir)/../../../libk3bdevice \
		-I$(srcdir)/../../../src \
		-I$(srcdir)/../../tools \
		-I$(srcdir)/.. \
		-I$(srcdir)/../datacd \
		-I$(srcdir)/../audiocd \
		-I$(srcdir)/../../plugin \
		-I../datacd \
		-I../audiocd \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libmixed.la

libmixed_la_SOURCES =	k3bmixeddoc.cpp \
			k3bmixedjob.cpp

include_HEADERS = 	k3bmixeddoc.h \
			k3bmixedjob.h
