INCLUDES = $(all_includes) -I$(srcdir)/libmad -I$(srcdir)/../../..

SUBDIRS = libmad

METASOURCES = AUTO

noinst_LTLIBRARIES = libinputmp3.la

libinputmp3_la_SOURCES = k3bmp3module.cpp

libinputmp3_la_LIBADD = libmad/libmad.la

