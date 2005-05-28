# we need the ../datacd for the uic generated header files
AM_CPPFLAGS=	-I$(srcdir)/../../core \
		-I$(srcdir)/../../../libk3bdevice \
		-I$(srcdir)/../../../src \
		-I$(srcdir)/../../tools \
		-I$(srcdir)/../datacd \
		-I$(srcdir)/.. \
		-I../datacd \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libmovix.la

libmovix_la_SOURCES = 	k3bmovixprogram.cpp \
			k3bmovixdoc.cpp \
			k3bmovixjob.cpp \
			k3bmovixfileitem.cpp \
			k3bmovixdocpreparer.cpp

include_HEADERS = 	k3bmovixdoc.h \
			k3bmovixjob.h \
			k3bmovixfileitem.h
