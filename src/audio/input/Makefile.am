INCLUDES = -I$(top_builddir)/k3b/src/audio/input/mp3 $(all_includes) -I$(srcdir)/../..

METASOURCES = AUTO

noinst_LTLIBRARIES = libinput.la

libinput_la_SOURCES = k3baudiomodule.cpp k3baudiomodulefactory.cpp 

libinput_la_LIBADD = mp3/libinputmp3.la ogg/libinputogg.la wave/libinputwave.la

SUBDIRS = mp3 ogg wave
