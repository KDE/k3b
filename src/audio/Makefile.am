INCLUDES = $(all_includes) -I$(srcdir)/.. -I$(srcdir)/../core

METASOURCES = AUTO

noinst_LTLIBRARIES = libaudio.la

libaudio_la_SOURCES = k3baudiojob.cpp k3baudiotrackdialog.cpp k3baudioburndialog.cpp k3baudioview.cpp k3baudiotrack.cpp k3baudiodoc.cpp audiolistviewitem.cpp audiolistview.cpp k3baudiodecoder.cpp k3baudiotocfilewriter.cpp k3baudiocdtextwidget.cpp

libaudio_la_LIBADD = input/libinput.la

SUBDIRS = input 
