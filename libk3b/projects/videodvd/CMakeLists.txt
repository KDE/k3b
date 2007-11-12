
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############


########### install files ###############

install( FILES  k3bvideodvddoc.h k3bvideodvdjob.h DESTINATION ${INCLUDE_INSTALL_DIR} )



#original Makefile.am contents follow:

## we need the ../datacd for the uic generated header files
#AM_CPPFLAGS= 	-I$(srcdir)/../../core \
#		-I$(srcdir)/../../../libk3bdevice \
#		-I$(srcdir)/../../../src \
#		-I$(srcdir)/../../tools \
#		-I$(srcdir)/../datadvd \
#		-I$(srcdir)/../datacd \
#		-I$(srcdir)/.. \
#		-I../datacd \
#		$(all_includes)
#
#METASOURCES = AUTO
#
#noinst_LTLIBRARIES = libvideodvd.la
#
#libvideodvd_la_SOURCES = 	k3bvideodvddoc.cpp \
#				k3bvideodvdjob.cpp \
#				k3bvideodvdimager.cpp
#
#include_HEADERS = k3bvideodvddoc.h k3bvideodvdjob.h
