INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../../libk3b/core -I$(srcdir)/../../libk3bdevice -I$(srcdir)/../../libk3b/projects -I$(srcdir)/../../libk3b/tools $(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libimagewriting.la

libimagewriting_la_SOURCES = k3bisoimagewritingdialog.cpp k3biso9660imagewritingjob.cpp k3bbinimagewritingjob.cpp k3bisoimageverificationjob.cpp k3bcdimagewritingdialog.cpp k3bclonetocreader.cpp k3bnrgfilereader.cpp

libimagewriting_la_LIBADD = ../cdcopy/libcdcopy.la
