# we need the ../datacd for the uic generated header files
AM_CPPFLAGS= 	-I$(srcdir)/../../core \
		-I$(top_srcdir)/libk3bdevice \
		-I$(srcdir)/../../tools \
		-I$(srcdir)/../datacd \
		-I$(srcdir)/.. \
		-I../datacd \
		$(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libdvd.la

libdvd_la_SOURCES = 	k3bdvddoc.cpp \
			k3bdvdjob.cpp \
			k3bdvdview.cpp \
			k3bdvdburndialog.cpp \
			k3bgrowisofsimager.cpp \
			k3bdvdformattingjob.cpp \
			k3bdvdformattingdialog.cpp \
			k3bdvdbooktypejob.cpp

include_HEADERS = k3bdvddoc.h k3bdvdjob.h k3bdvdview.h k3bdvdformattingjob.h
