
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

add_subdirectory( core )
add_subdirectory( plugin )
add_subdirectory( tools )
add_subdirectory( projects )
add_subdirectory( cddb )
add_subdirectory( jobs )

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(VIDEODVDDIR)")

########### next target ###############

set(k3b_LIB_SRCS dummy.cpp )

kde4_add_library(k3b SHARED ${k3b_LIB_SRCS})

target_link_libraries(k3b  ${KDE4_KDECORE_LIBS} k3bcore cddb k3bproject k3bplugin k3btools jobs k3bdevice )

set_target_properties(k3b PROPERTIES VERSION 3.0.0 SOVERSION 3 )
install(TARGETS k3b DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############




#original Makefile.am contents follow:

#if include_videodvdrip
#VIDEODVDDIR = videodvd
#VIDEODVDLIB = videodvd/libvideodvd.la
#endif
#
#
#lib_LTLIBRARIES = libk3b.la
#
#libk3b_la_SOURCES = dummy.cpp
#
#libk3b_la_LIBADD = core/libk3bcore.la \
#	cddb/libcddb.la \
#	projects/libk3bproject.la \
#	plugin/libk3bplugin.la \
#	tools/libk3btools.la \
#	jobs/libjobs.la \
#	$(VIDEODVDLIB) \
#	../libk3bdevice/libk3bdevice.la
#
#libk3b_la_LDFLAGS = $(all_libraries) -version-info 3:0:0 -no-undefined
#
#SUBDIRS = core plugin tools projects cddb jobs $(VIDEODVDDIR)
#
##pkgconfigdir = $(libdir)/pkgconfig
##pkgconfig_DATA = libk3b.pc
#
#messages:
#	$(XGETTEXT) `find -name "*.cpp" -o -name "*.h"` -o $(podir)/libk3b.pot
