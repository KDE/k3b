INCLUDES = $(all_includes) -I$(srcdir)/..

METASOURCES = AUTO

noinst_LTLIBRARIES = libvcd.la

libvcd_la_SOURCES = k3bvcddoc.cpp k3bvcdtrack.cpp k3bvcdjob.cpp k3bvcdview.cpp k3bvcdlistviewitem.cpp k3bvcdlistview.cpp k3bvcdburndialog.cpp k3bvcdoptions.cpp k3bvcdtrackdialog.cpp

libvcd_la_LIBADD = mpeginfo/libmpeginfo.la

SUBDIRS = mpeginfo
