AM_CPPFLAGS= -Imp3 -I$(srcdir)/../.. $(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libinput.la

libinput_la_SOURCES = k3baudiomodule.cpp k3baudiomodulefactory.cpp 

libinput_la_LIBADD = mp3/libinputmp3.la ogg/libinputogg.la wave/libinputwave.la
# we need this
libinput_la_COMPILE_FIRST = mp3/libmad/mad.h

SUBDIRS = mp3 ogg wave
