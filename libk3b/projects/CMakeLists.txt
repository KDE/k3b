

add_subdirectory( datacd )
add_subdirectory( audiocd )
add_subdirectory( mixedcd )
add_subdirectory( movixcd )
add_subdirectory( videocd )
add_subdirectory( videodvd )


########### next target ###############


########### install files ###############

install( FILES  k3bdoc.h  	                   k3bgrowisofswriter.h                             k3bcdrdaowriter.h                             k3bcdrecordwriter.h  			   k3binffilewriter.h  			   k3btocfilewriter.h  			   k3bcuefileparser.h  			   k3bimagefilereader.h DESTINATION ${INCLUDE_INSTALL_DIR} )



#original Makefile.am contents follow:

#AM_CPPFLAGS= -I$(srcdir)/../../src -I$(srcdir)/../core -I$(srcdir)/../plugin -I$(srcdir)/../../libk3bdevice -I$(srcdir)/../tools -I$(srcdir)/datacd -I$(srcdir)/audiocd -I$(srcdir)/mixedcd -I$(srcdir)/movixcd -I$(srcdir)/videocd -I$(srcdir)/videodvd $(all_includes)
#
#noinst_LTLIBRARIES = libk3bproject.la
#
#libk3bproject_la_LIBADD = ./datacd/libdata.la ./mixedcd/libmixed.la ./videocd/libvcd.la ./videodvd/libvideodvd.la ./movixcd/libmovix.la ./audiocd/libaudio.la
#
#libk3bproject_la_LDFLAGS = $(all_libraries)
#
#libk3bproject_la_SOURCES = k3babstractwriter.cpp \
#	                   k3bgrowisofswriter.cpp \
#	                   k3bgrowisofshandler.cpp \
#                           k3bdoc.cpp \
#                           k3bcdrdaowriter.cpp \
#                           k3bcdrecordwriter.cpp \
#			   k3binffilewriter.cpp \
#			   k3btocfilewriter.cpp \
#			   k3bimagefilereader.cpp \
#			   k3bcuefileparser.cpp \
#			   k3bpipebuffer.cpp
#
#include_HEADERS =          k3bdoc.h \
#	                   k3bgrowisofswriter.h \
#                           k3bcdrdaowriter.h \
#                           k3bcdrecordwriter.h \
#			   k3binffilewriter.h \
#			   k3btocfilewriter.h \
#			   k3bcuefileparser.h \
#			   k3bimagefilereader.h
#
#SUBDIRS = datacd audiocd mixedcd movixcd videocd videodvd
#
#METASOURCES = AUTO
