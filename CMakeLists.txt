project(k3b)

set( CMAKE_MODULE_PATH    ${CMAKE_MODULE_PATH}
                          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)
include(MacroLogFeature)
find_package(DvdRead REQUIRED)
macro_optional_find_package(MusicBrainz)
find_package(Kcddb REQUIRED)
find_package(Taglib)

macro_log_feature( TAGLIB_FOUND "taglib" "Read and write tags in audio files" "http://www.freshmeat.net/projects/taglib" TRUE "1.4.0" "")

macro_log_feature( MUSICBRAINZ_FOUND "musicbrainz" "Provide information about the CD, about the artist or about related information" "http://musicbrainz.org/" TRUE "" "") 

macro_log_feature( DVDREAD_FOUND "dvdread" "Libdvdread provides a simple foundation for reading DVD video disks." "http://www.dtek.chalmers.se/groups/dvd/downloads.shtml" TRUE "" "")

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

include(ConfigureChecks.cmake)

include_directories( 
  ${KDE4_INCLUDES}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/libk3bdevice
  ${CMAKE_CURRENT_BINARY_DIR}/libk3bdevice )

SET(K3B_VERSION_STRING "1.95-svn")

if(DVDREAD_FOUND)
  set(HAVE_LIBDVDREAD 1)
endif(DVDREAD_FOUND)

if(MUSICBRAINZ_FOUND)
  set(HAVE_MUSICBRAINZ 1)
endif(MUSICBRAINZ_FOUND)

if(TAGLIB_FOUND)
  set(HAVE_TAGLIB 1)
endif(TAGLIB_FOUND)

configure_file (config-k3b.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-k3b.h )
if(NOT WIN32)
   configure_file (k3b.lsm.cmake ${CMAKE_CURRENT_BINARY_DIR}/k3b.lsm )
endif(NOT WIN32)

option(K3BSETUP_BUILD "Build k3bsetup" ON)
option(ADD_K3B_DEBUG "K3b additional debugging support")


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


# apidox
find_package(Doxygen)

#laurent: removes this line when Doxyfile.cmake will add to svn
set(DOXYGEN_EXECUTABLE FALSE)
if(DOXYGEN_EXECUTABLE)
  configure_file(${k3b_SOURCE_DIR}/Doxyfile.cmake ${k3b_BINARY_DIR}/Doxyfile)

  if(EXISTS ${QT_DOC_DIR}/html)
    set(QTDOCS "${QT_DOC_DIR}/html")
  else(EXISTS ${QT_DOC_DIR}/html)
    set(QTDOCS "http://doc.trolltech.com/4.3/")
  endif(EXISTS ${QT_DOC_DIR}/html)

  add_custom_target(
    apidox
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
    COMMAND docs/html/installdox -l qt4.tag@${QTDOCS} docs/html/*.html)
endif(DOXYGEN_EXECUTABLE)

macro_display_feature_log()

