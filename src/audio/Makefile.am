AM_CPPFLAGS= -I$(srcdir)/.. -I$(srcdir)/../core $(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libaudio.la

libaudio_la_SOURCES = base_k3baudiocdtextwidget.ui k3baudiojob.cpp k3baudiotrackdialog.cpp k3baudioburndialog.cpp k3baudioview.cpp k3baudiotrack.cpp k3baudiodoc.cpp audiolistviewitem.cpp audiolistview.cpp k3baudiocdtextwidget.cpp k3baudionormalizejob.cpp k3baudiojobtempdata.cpp k3baudiostreamer.cpp

libaudio_la_LIBADD = input/libinput.la

SUBDIRS = input 
