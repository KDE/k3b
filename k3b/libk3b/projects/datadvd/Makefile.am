# we need the ../datacd for the uic generated header files
AM_CPPFLAGS= 	-I$(srcdir)/../../core \
		-I$(srcdir)/../../../libk3bdevice \
		-I$(srcdir)/../../../src \
		-I$(srcdir)/../../tools \
		-I$(srcdir)/../../jobs \
		-I$(srcdir)/../datacd \
		-I$(srcdir)/.. \
		-I../datacd \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libdvd.la

libdvd_la_SOURCES = 	k3bdvddoc.cpp \
			k3bdvdjob.cpp \
			k3bdvdbooktypejob.cpp

include_HEADERS = 	k3bdvddoc.h \
			k3bdvdjob.h
