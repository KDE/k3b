project(k3b)

find_package(KDE4 REQUIRED)

set( CMAKE_MODULE_PATH    ${CMAKE_MODULE_PATH}
                          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include(ConvenienceLibs.cmake)

#include(ConfigureChecks.cmake)

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}
${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/libk3bdevice
${CMAKE_CURRENT_BINARY_DIR}/libk3bdevice )

configure_file (config-k3b.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-k3b.h )
option(K3BSETUP_BUILD "Build k3bsetup")


add_subdirectory( libk3bdevice )
add_subdirectory( libk3b )
add_subdirectory( src )
add_subdirectory( kioslaves )
add_subdirectory( plugins )
# TODO port to strigi
#add_subdirectory( kfile-plugins )
if(K3BSETUP_BUILD)
   add_subdirectory(k3bsetup)
endif(K3BSETUP_BUILD)

########### install files ###############




#original Makefile.am contents follow:

#if with_k3bsetup1
#  K3BSETUPDIR=k3bsetup
#endif
#
#EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO k3b.lsm admin configure.in.in
#
#AUTOMAKE_OPTIONS = foreign 1.5
#
##include admin/deps.am
#
#SUBDIRS = libk3bdevice libk3b src kioslaves plugins kfile-plugins $(K3BSETUPDIR)
