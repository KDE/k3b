# we need the ../datacd and ../movixcd for the uic generated header files
AM_CPPFLAGS= 	-I$(srcdir)/../../core \
		-I$(srcdir)/../../../libk3bdevice \
		-I$(srcdir)/../../../src \
		-I$(srcdir)/../../tools \
		-I$(srcdir)/../datadvd \
		-I$(srcdir)/../movixcd \
		-I$(srcdir)/../datacd \
		-I$(srcdir)/.. \
		-I../datacd \
		-I../movixcd \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libmovixdvd.la

libmovixdvd_la_SOURCES = k3bmovixdvddoc.cpp k3bmovixdvdjob.cpp

include_HEADERS = 	k3bmovixdvddoc.h \
			k3bmovixdvdjob.h
