project(k3b)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

##################  K3b version  ################################
set(K3B_VERSION_MAJOR   2)
set(K3B_VERSION_MINOR   0)
set(K3B_VERSION_RELEASE 80)
set(K3B_VERSION_STRING "${K3B_VERSION_MAJOR}.${K3B_VERSION_MINOR}.${K3B_VERSION_RELEASE}")

# increase on BC breakage
set(K3B_LIB_VERSION_MAJOR 6)

# increase after adding functionality
set(K3B_LIB_VERSION_MINOR 0)
set(K3B_LIB_VERSION_RELEASE 0)


##################  K3b options #################################
option(K3B_DEBUG "K3b additional debugging support")
option(K3B_ENABLE_MUSICBRAINZ "Support for querying metadata about audio tracks from Musicbrainz." ON)
option(K3B_ENABLE_DVD_RIPPING "Support for ripping Video DVDs with optional decryption." ON)
option(K3B_ENABLE_TAGLIB "Support for reading audio file metadata using Taglib." ON)
option(K3B_BUILD_API_DOCS "Build the API documentation for the K3b libs." OFF)
if(NOT WIN32 AND NOT APPLE)
  option(K3B_BUILD_K3BSETUP "Graphical utility for managing device permissions" ON)
  option(K3B_ENABLE_HAL_SUPPORT "Support for inhibiting HAL from polling disk during write." OFF)
endif(NOT WIN32 AND NOT APPLE)

# plugin options
option(K3B_BUILD_FFMPEG_DECODER_PLUGIN "Build FFmpeg decoder plugin" ON)
option(K3B_BUILD_OGGVORBIS_DECODER_PLUGIN "Build Ogg-Vorbis decoder plugin" ON)
option(K3B_BUILD_OGGVORBIS_ENCODER_PLUGIN "Build Ogg-Vorbis encoder plugin" ON)
option(K3B_BUILD_MAD_DECODER_PLUGIN "Build MAD mp3 decoder plugin" ON)
option(K3B_BUILD_MUSE_DECODER_PLUGIN "Build Musepack decoder plugin" ON)
option(K3B_BUILD_FLAC_DECODER_PLUGIN "Build Flac decoder plugin" ON)
option(K3B_BUILD_SNDFILE_DECODER_PLUGIN "Build libsndfile decoder plugin" ON)
option(K3B_BUILD_LAME_ENCODER_PLUGIN "Build Lame encoder plugin" ON)
option(K3B_BUILD_SOX_ENCODER_PLUGIN "Build Sox encoder plugin" ON)
option(K3B_BUILD_EXTERNAL_ENCODER_PLUGIN "Build external app encoder plugin" ON)
option(K3B_BUILD_WAVE_DECODER_PLUGIN "Build Wave decoder plugin" ON)

##################  K3b requirements #################################
set(QT_MIN_VERSION 4.6.0)
set(KDE_MIN_VERSION 4.4.0)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroLogFeature)
macro_log_feature( KDE4_FOUND "kdelibs"
    "The toolkit K3b uses to build"
    "http://www.kde.org"
    TRUE ${KDE_MIN_VERSION} "" )
macro_log_feature( QT_QTMULTIMEDIA_FOUND "QtMultimedia"
    "Low-level multimedia library used for previewing audio projects"
    "http://doc.qt.nokia.com/latest/qtmultimedia.html"
    FALSE )

if(K3B_ENABLE_DVD_RIPPING)
  find_package(DvdRead)
  macro_log_feature( DVDREAD_FOUND "libDVDRead"
    "Libdvdread provides a simple foundation for reading DVD video disks."
    "http://www.dtek.chalmers.se/groups/dvd/downloads.shtml"
    FALSE "" "")
endif(K3B_ENABLE_DVD_RIPPING)

if(K3B_ENABLE_TAGLIB)
  find_package(Taglib)
  macro_log_feature( TAGLIB_FOUND "Taglib"
    "Read and write tags in audio files"
    "http://www.freshmeat.net/projects/taglib"
    FALSE "1.4.0" "")
endif(K3B_ENABLE_TAGLIB)

find_package(Samplerate)
macro_log_feature( SAMPLERATE_FOUND "Samplerate"
  "Audio sampling rate conversion."
  "http://www.mega-nerd.com/SRC/"
  TRUE "" "")

if(K3B_ENABLE_MUSICBRAINZ)
  find_package(MusicBrainz)
  macro_log_feature( MUSICBRAINZ_FOUND "Musicbrainz"
    "Provide information about the CD, about the artist or about related information"
    "http://musicbrainz.org/"
    FALSE "1.1" "")
endif(K3B_ENABLE_MUSICBRAINZ)

find_package(Kcddb)
macro_log_feature( KCDDB_FOUND "KCddb"
  "KCddb is used to retrieve audio CD meta data from the internet."
  "http://multimedia.kde.org"
  TRUE "" "")

if(K3B_BUILD_FFMPEG_DECODER_PLUGIN)
  find_package(FFmpeg)
  macro_log_feature( FFMPEG_FOUND "FFmpeg"
    "Needed for the K3b FFmpeg decoder plugin which can decode virtually all audio types."
    "http://ffmpeg.org/"
    FALSE "" "")
endif(K3B_BUILD_FFMPEG_DECODER_PLUGIN)

if(K3B_BUILD_FLAC_DECODER_PLUGIN)
  find_package(Flac)
  find_package(Flac++)
  macro_log_feature( FLAC_FOUND "Flac"
    "Needed for the Flac audio decoder plugin."
    "http://flac.sourceforge.net/"
    FALSE "" "")
  macro_log_feature( FLAC++_FOUND "Flac++"
    "Needed for the Flac audio decoder plugin."
    "http://flac.sourceforge.net/"
    FALSE "" "")
endif(K3B_BUILD_FLAC_DECODER_PLUGIN)

if(K3B_BUILD_MAD_DECODER_PLUGIN)
  find_package(Mad)
  macro_log_feature( MAD_FOUND "Mad mp3"
    "Needed for the mp3 audio decoder plugin."
    "http://www.underbit.com/products/mad/"
    FALSE "" "")
endif(K3B_BUILD_MAD_DECODER_PLUGIN)

if(K3B_BUILD_MUSE_DECODER_PLUGIN)
  find_package(Muse)
  macro_log_feature( MUSE_FOUND "Muse"
    "Needed for the Musepack audio decoder plugin"
    "http://www.musepack.net/"
    FALSE "" "")
endif(K3B_BUILD_MUSE_DECODER_PLUGIN)

if(K3B_BUILD_SNDFILE_DECODER_PLUGIN)
  find_package(Sndfile)
  macro_log_feature( SNDFILE_FOUND "Sndfile"
    "Needed for the libsndfile audio decoder plugin."
    "http://www.mega-nerd.com/libsndfile/"
    FALSE "" "")
endif(K3B_BUILD_SNDFILE_DECODER_PLUGIN)

if(K3B_BUILD_LAME_ENCODER_PLUGIN)
  find_package(Lame)
  macro_log_feature( LAME_FOUND "Lame mp3 encoder"
    "Needed for the lame mpf encoder encoder plugin."
    "http://lame.sourceforge.net/"
    FALSE "" "")
endif(K3B_BUILD_LAME_ENCODER_PLUGIN)

if(K3B_BUILD_OGGVORBIS_DECODER_PLUGIN OR K3B_BUILD_OGGVORBIS_ENCODER_PLUGIN)
  find_package(OggVorbis)
  macro_log_feature( OGGVORBIS_FOUND "Ogg Vorbis"
    "Needed for the K3b Ogg Vorbis decoder and encoder plugins."
    "http://www.vorbis.com/"
    FALSE "" "")
endif(K3B_BUILD_OGGVORBIS_DECODER_PLUGIN OR K3B_BUILD_OGGVORBIS_ENCODER_PLUGIN)


if (WIN32)
find_library(SAMPLERATE_LIBRARIES samplerate-0)
endif(WIN32)

##################  K3b build settings #################################
include(ConfigureChecks)

if(K3B_BUILD_K3BSETUP)
  set(BUILD_K3BSETUP 1)
endif(K3B_BUILD_K3BSETUP)

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

if(K3B_BUILD_FFMPEG_DECODER_PLUGIN AND FFMPEG_FOUND)
  set(BUILD_FFMPEG_DECODER_PLUGIN 1)
endif(K3B_BUILD_FFMPEG_DECODER_PLUGIN AND FFMPEG_FOUND)

if(K3B_BUILD_FLAC_DECODER_PLUGIN AND FLAC_FOUND AND FLAC++_FOUND)
  set(BUILD_FLAC_DECODER_PLUGIN 1)
endif(K3B_BUILD_FLAC_DECODER_PLUGIN AND FLAC_FOUND AND FLAC++_FOUND)

if(K3B_BUILD_MAD_DECODER_PLUGIN AND MAD_FOUND)
  set(BUILD_MAD_DECODER_PLUGIN 1)
endif(K3B_BUILD_MAD_DECODER_PLUGIN AND MAD_FOUND)

if(K3B_BUILD_MUSE_DECODER_PLUGIN AND MUSE_FOUND)
  set(BUILD_MUSE_DECODER_PLUGIN 1)
endif(K3B_BUILD_MUSE_DECODER_PLUGIN AND MUSE_FOUND)

if(K3B_BUILD_SNDFILE_DECODER_PLUGIN AND SNDFILE_FOUND)
  set(BUILD_SNDFILE_DECODER_PLUGIN 1)
endif(K3B_BUILD_SNDFILE_DECODER_PLUGIN AND SNDFILE_FOUND)

if(K3B_BUILD_LAME_ENCODER_PLUGIN AND LAME_FOUND)
  set(BUILD_LAME_ENCODER_PLUGIN 1)
endif(K3B_BUILD_LAME_ENCODER_PLUGIN AND LAME_FOUND)

if(K3B_BUILD_OGGVORBIS_DECODER_PLUGIN AND OGGVORBIS_FOUND)
  set(BUILD_OGGVORBIS_DECODER_PLUGIN 1)
endif(K3B_BUILD_OGGVORBIS_DECODER_PLUGIN AND OGGVORBIS_FOUND)

if(K3B_BUILD_OGGVORBIS_ENCODER_PLUGIN AND OGGVORBIS_FOUND)
  set(BUILD_OGGVORBIS_ENCODER_PLUGIN 1)
endif(K3B_BUILD_OGGVORBIS_ENCODER_PLUGIN AND OGGVORBIS_FOUND)

if(K3B_BUILD_WAVE_DECODER_PLUGIN)
  set(BUILD_WAVE_DECODER_PLUGIN 1)
endif(K3B_BUILD_WAVE_DECODER_PLUGIN)

if(K3B_BUILD_SOX_ENCODER_PLUGIN)
 set(BUILD_SOX_ENCODER_PLUGIN 1)
endif(K3B_BUILD_SOX_ENCODER_PLUGIN)

if(K3B_BUILD_EXTERNAL_ENCODER_PLUGIN)
 set(BUILD_EXTERNAL_ENCODER_PLUGIN 1)
endif(K3B_BUILD_EXTERNAL_ENCODER_PLUGIN)

if(K3B_ENABLE_HAL_SUPPORT)
  set(ENABLE_HAL_SUPPORT 1)
endif(K3B_ENABLE_HAL_SUPPORT)

if(QT_QTMULTIMEDIA_FOUND)
    set(ENABLE_AUDIO_PLAYER 1)
endif(QT_QTMULTIMEDIA_FOUND)

configure_file (config-k3b.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-k3b.h )


##################  K3b apidox ################################
if(K3B_BUILD_API_DOCS)
  find_package(Doxygen)

  if(DOXYGEN_EXECUTABLE)
    configure_file(${k3b_SOURCE_DIR}/Doxyfile.cmake ${k3b_BINARY_DIR}/Doxyfile)

    if(EXISTS ${QT_DOC_DIR}/html)
      set(QTDOCS "${QT_DOC_DIR}/html")
    else(EXISTS ${QT_DOC_DIR}/html)
      set(QTDOCS "http://doc.trolltech.com/4.5/")
    endif(EXISTS ${QT_DOC_DIR}/html)

    add_custom_target(
      apidox
      COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
      COMMAND docs/html/installdox -l qt4.tag@${QTDOCS} docs/html/*.html)
  endif(DOXYGEN_EXECUTABLE)
endif(K3B_BUILD_API_DOCS)


##################  K3b build environment ################################

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(
  ${k3b_BINARY_DIR}
  ${k3b_SOURCE_DIR}/libk3bdevice
  ${k3b_SOURCE_DIR}/libk3b/core
  ${k3b_SOURCE_DIR}/libk3b/plugin
  ${k3b_SOURCE_DIR}/libk3b/tools
  ${k3b_SOURCE_DIR}/libk3b/tools/qprocess
  ${k3b_SOURCE_DIR}/libk3b/tools/qprocess/private
  ${k3b_SOURCE_DIR}/libk3b/projects
  ${k3b_SOURCE_DIR}/libk3b/projects/audiocd
  ${k3b_SOURCE_DIR}/libk3b/projects/datacd
  ${k3b_SOURCE_DIR}/libk3b/projects/videocd
  ${k3b_SOURCE_DIR}/libk3b/projects/mixedcd
  ${k3b_SOURCE_DIR}/libk3b/jobs
  ${k3b_SOURCE_DIR}/libk3b/videodvd
  ${KDE4_INCLUDES}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
)

add_subdirectory( libk3bdevice )
add_subdirectory( libk3b )
add_subdirectory( src )
add_subdirectory( kioslaves )
add_subdirectory( plugins )
add_subdirectory( doc )
if(BUILD_K3BSETUP)
    add_subdirectory(k3bsetup)
endif(BUILD_K3BSETUP)
if(KDE4_BUILD_TESTS)
    enable_testing()
    add_subdirectory( tests )
endif(KDE4_BUILD_TESTS)


##################  K3b config summary ################################
macro_display_feature_log()
