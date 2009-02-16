project(k3b)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)


##################  K3b version  ################################
set(K3B_VERSION_MAJOR 1)
set(K3B_VERSION_MINOR 60)
set(K3B_VERSION_RELEASE 0)
set(K3B_VERSION_STRING "${K3B_VERSION_MAJOR}.${K3B_VERSION_MINOR}.${K3B_VERSION_RELEASE}")

# increase on BC breakage
set(K3B_LIB_VERSION_MAJOR 6)

# increase after adding functionality
set(K3B_LIB_VERSION_MINOR 0)
set(K3B_LIB_VERSION_RELEASE 0)


##################  K3b options #################################
option(K3B_BUILD_K3BSETUP "Build k3bsetup" ON)
option(K3B_DEBUG "K3b additional debugging support")
option(K3B_ENABLE_MUSICBRAINZ "Support for querying metadata about audio tracks from Musicbrainz." ON)
option(K3B_ENABLE_DVD_RIPPING "Support for ripping Video DVDs with optional decryption." ON)
option(K3B_ENABLE_TAGLIB "Support for reading audio file metadata using Taglib." ON)


##################  K3b requirements #################################
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroLogFeature)

if(K3B_ENABLE_DVD_RIPPING)
  macro_optional_find_package(DvdRead)
  macro_log_feature( DVDREAD_FOUND "dvdread"
    "Libdvdread provides a simple foundation for reading DVD video disks."
    "http://www.dtek.chalmers.se/groups/dvd/downloads.shtml"
    TRUE
    ""
    "")
endif(K3B_ENABLE_DVD_RIPPING)

if(K3B_ENABLE_TAGLIB)
  macro_optional_find_package(Taglib)
  macro_log_feature( TAGLIB_FOUND "taglib"
    "Read and write tags in audio files"
    "http://www.freshmeat.net/projects/taglib"
    TRUE
    "1.4.0"
    "")
endif(K3B_ENABLE_TAGLIB)

if(K3B_ENABLE_MUSICBRAINZ)
  macro_optional_find_package(MusicBrainz)
  macro_log_feature( MUSICBRAINZ_FOUND "musicbrainz"
    "Provide information about the CD, about the artist or about related information"
    "http://musicbrainz.org/"
    FALSE
    "1.1"
    "") 
endif(K3B_ENABLE_MUSICBRAINZ)

find_package(Kcddb REQUIRED)
macro_log_feature( KCDDB_FOUND "kcddb"
  "KCddb is used to retrieve audio CD meta data from the internet." 
  "http://multimedia.kde.org" 
  TRUE
  ""
  "")


##################  K3b build settings #################################
include(ConfigureChecks.cmake)

macro_bool_to_01(K3B_BUILD_K3BSETUP BUILD_K3BSETUP)
macro_bool_to_01(ADD_K3B_DEBUG K3B_DEBUG)
if(K3B_ENABLE_DVD_RIPPING AND DVDREAD_FOUND)
  set(ENABLE_DVD_RIPPING 1)
endif(K3B_ENABLE_DVD_RIPPING AND DVDREAD_FOUND)
if(K3B_ENABLE_TAGLIB AND TAGLIB_FOUND)
  set(ENABLE_TAGLIB 1)
endif(K3B_ENABLE_TAGLIB AND TAGLIB_FOUND)
if(K3B_ENABLE_MUSICBRAINZ AND MUSICBRAINZ_FOUND)
  set(ENABLE_MUSICBRAINZ 1)
endif(K3B_ENABLE_MUSICBRAINZ AND MUSICBRAINZ_FOUND)

configure_file (config-k3b.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-k3b.h )
if(NOT WIN32)
   configure_file(k3b.lsm.cmake ${CMAKE_CURRENT_BINARY_DIR}/k3b.lsm )
endif(NOT WIN32)


##################  K3b apidox ################################
# trueg: disabled for now
if(0)
find_package(Doxygen)

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
endif(0)


##################  K3b build environment ################################

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

include_directories( 
  ${KDE4_INCLUDES}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/libk3bdevice
  ${CMAKE_CURRENT_BINARY_DIR}/libk3bdevice )

add_subdirectory( libk3bdevice )
add_subdirectory( libk3b )
add_subdirectory( src )
add_subdirectory( kioslaves )
add_subdirectory( plugins )
# TODO port to strigi
#add_subdirectory( kfile-plugins )
if(K3B_BUILD_K3BSETUP)
   add_subdirectory(k3bsetup)
endif(K3B_BUILD_K3BSETUP)


##################  K3b config summary ################################
macro_display_feature_log()
