AM_CPPFLAGS= -I$(srcdir)/libmad -I$(srcdir)/../../.. $(all_includes)

SUBDIRS = libmad

METASOURCES = AUTO

noinst_LTLIBRARIES = libinputmp3.la

libinputmp3_la_SOURCES = k3bmp3module.cpp
libinputmp3_la_LIBADD = libmad/libmad.la
# we need this dependency for non-recursive make
libinputmp3_la_COMPILE_FIRST = libmad/mad.h

