project(k3b)

cmake_minimum_required(VERSION 2.8.12)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH "${ECM_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

##################  K3b version  ################################
set(K3B_VERSION_MAJOR   2)
set(K3B_VERSION_MINOR   9)
set(K3B_VERSION_RELEASE 90)
set(K3B_VERSION_STRING "${K3B_VERSION_MAJOR}.${K3B_VERSION_MINOR}.${K3B_VERSION_RELEASE}")

# increase on BC breakage
set(K3B_LIB_VERSION_MAJOR 7)

# increase after adding functionality
set(K3B_LIB_VERSION_MINOR 0)
set(K3B_LIB_VERSION_RELEASE 0)


##################  K3b options #################################
option(K3B_DEBUG "K3b additional debugging support")
option(K3B_ENABLE_MUSICBRAINZ "Support for querying metadata about audio tracks from Musicbrainz." ON)
option(K3B_ENABLE_DVD_RIPPING "Support for ripping Video DVDs with optional decryption." ON)
option(K3B_ENABLE_TAGLIB "Support for reading audio file metadata using Taglib." ON)
option(K3B_BUILD_API_DOCS "Build the API documentation for the K3b libs." OFF)

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
include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
include(FeatureSummary)
include(GenerateExportHeader)
include(ECMInstallIcons)

find_package(Qt5 REQUIRED COMPONENTS Core DBus Gui Test WebKitWidgets)
find_package(KF5 REQUIRED COMPONENTS Archive Config DocTools I18n KCMUtils KDELibs4Support KIO NotifyConfig Solid WidgetsAddons)

#find_package(KDE4 REQUIRED)
#include(KDE4Defaults)
#include(MacroLibrary)
#include(MacroLogFeature)

#macro_log_feature( KDE4_FOUND "kdelibs"
#    "The toolkit K3b uses to build"
#    "http://www.kde.org"
#    TRUE ${KDE_MIN_VERSION} "" )
#macro_log_feature( QT_QTMULTIMEDIA_FOUND "QtMultimedia"
#    "Low-level multimedia library used for previewing audio projects"
#    "http://doc.qt.nokia.com/latest/qtmultimedia.html"
#    FALSE )

#find_package(Iconv)
#if(ICONV_FOUND)
#  set(HAVE_ICONV 1)
#endif(ICONV_FOUND)

#if(K3B_ENABLE_DVD_RIPPING)
#  find_package(DvdRead)
#  macro_log_feature( DVDREAD_FOUND "libDVDRead"
#    "Libdvdread provides a simple foundation for reading DVD video disks."
#    "http://www.dtek.chalmers.se/groups/dvd/downloads.shtml"
#    FALSE "" "")
#endif(K3B_ENABLE_DVD_RIPPING)

#if(K3B_ENABLE_TAGLIB)
#  find_package(Taglib)
#  macro_log_feature( TAGLIB_FOUND "Taglib"
#    "Read and write tags in audio files"
#    "http://www.freshmeat.net/projects/taglib"
#    FALSE "1.4.0" "")
#endif(K3B_ENABLE_TAGLIB)

find_package(Samplerate)
set_package_properties(Samplerate PROPERTIES
   DESCRIPTION "Audio sampling rate conversion."
   URL "http://www.mega-nerd.com/SRC"
   TYPE REQUIRED)

#if(K3B_ENABLE_MUSICBRAINZ)
#  find_package(MusicBrainz)
#  macro_log_feature( MUSICBRAINZ_FOUND "Musicbrainz"
#    "Provide information about the CD, about the artist or about related information"
#    "http://musicbrainz.org/"
#    FALSE "1.1" "")
#endif(K3B_ENABLE_MUSICBRAINZ)

find_package(Libkcddb)
set_package_properties(Libkcddb PROPERTIES
   DESCRIPTION "KDE CDDB library"
   URL "http://projects.kde.org/projects/kde/kdemultimedia/libkcddb"
   TYPE REQUIRED
   PURPOSE "libkcddb is used to retrieve audio CD meta data from the internet.")

#if(K3B_BUILD_FFMPEG_DECODER_PLUGIN)
#  find_package(FFmpeg)
#  macro_log_feature( FFMPEG_FOUND "FFmpeg"
#    "Needed for the K3b FFmpeg decoder plugin which can decode virtually all audio types."
#    "http://ffmpeg.org/"
#    FALSE "" "")
#  if(FFMPEG_FOUND)
#    include(CMakePushCheckState)
#    cmake_push_check_state()

#    if(CMAKE_COMPILER_IS_GNUCC)
#      # FindKDE4Internal.cmake screws things up
#      set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} -std=c99)
#    endif(CMAKE_COMPILER_IS_GNUCC)

#    if(FFMPEG_INCLUDE_DIR_OLD_STYLE)
#      set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${FFMPEG_INCLUDE_DIR_OLD_STYLE})
#      set(FFMPEG_HEADERS ffmpeg/avcodec.h ffmpeg/avformat.h)
#    else(FFMPEG_INCLUDE_DIR_OLD_STYLE)
#      set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${FFMPEG_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIRS})
#      set(FFMPEG_HEADERS libavcodec/avcodec.h libavformat/avformat.h)
#      set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -DNEWFFMPEGAVCODECPATH)
#    endif(FFMPEG_INCLUDE_DIR_OLD_STYLE)
#    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${FFMPEG_LIBRARIES})

#    check_symbol_exists(avformat_open_input "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVFORMAT_OPEN_INPUT)
#    check_symbol_exists(av_dump_format "${FFMPEG_HEADERS}" HAVE_FFMPEG_AV_DUMP_FORMAT)
#    check_symbol_exists(avformat_find_stream_info "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVFORMAT_FIND_STREAM_INFO)
#    check_symbol_exists(avformat_close_input "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVFORMAT_CLOSE_INPUT)
#    check_symbol_exists(avcodec_open2 "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVCODEC_OPEN2)
#    check_symbol_exists(avcodec_decode_audio2 "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVCODEC_DECODE_AUDIO2)
#    check_symbol_exists(avcodec_decode_audio3 "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVCODEC_DECODE_AUDIO3)
#    check_symbol_exists(avcodec_decode_audio4 "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVCODEC_DECODE_AUDIO4)
#    include(CheckCSourceCompiles)
#    check_c_source_compiles("
#      #ifdef NEWFFMPEGAVCODECPATH
#      #include <libavcodec/avcodec.h>
#      #else
#      #include <ffmpeg/avcodec.h>
#      #endif
#      int main() { enum AVMediaType t = AVMEDIA_TYPE_UNKNOWN; return 0; }
#      " HAVE_FFMPEG_AVMEDIA_TYPE)
#    check_c_source_compiles("
#      #ifdef NEWFFMPEGAVCODECPATH
#      #include <libavcodec/avcodec.h>
#      #else
#      #include <ffmpeg/avcodec.h>
#      #endif
#      int main() {
#      #if LIBAVCODEC_BUILD >= AV_VERSION_INT(54,25,0)
#      # Not needed here. AV_CODEC_ID_MP3 exists.
#      #else
#      enum CodecID t = CODEC_ID_MP3;
#      #endif
#      return 0;
#      }
#      " HAVE_FFMPEG_CODEC_MP3)

#    cmake_pop_check_state()
#  endif(FFMPEG_FOUND)
#endif(K3B_BUILD_FFMPEG_DECODER_PLUGIN)

#if(K3B_BUILD_FLAC_DECODER_PLUGIN)
#  find_package(Flac)
#  find_package(Flac++)
#  macro_log_feature( FLAC_FOUND "Flac"
#    "Needed for the Flac audio decoder plugin."
#    "http://flac.sourceforge.net/"
#    FALSE "" "")
#  macro_log_feature( FLAC++_FOUND "Flac++"
#    "Needed for the Flac audio decoder plugin."
#    "http://flac.sourceforge.net/"
#    FALSE "" "")
#endif(K3B_BUILD_FLAC_DECODER_PLUGIN)

#if(K3B_BUILD_MAD_DECODER_PLUGIN)
#  find_package(Mad)
#  macro_log_feature( MAD_FOUND "Mad mp3"
#    "Needed for the mp3 audio decoder plugin."
#    "http://www.underbit.com/products/mad/"
#    FALSE "" "")
#endif(K3B_BUILD_MAD_DECODER_PLUGIN)

#if(K3B_BUILD_MUSE_DECODER_PLUGIN)
#  find_package(Muse)
#  macro_log_feature( MUSE_FOUND "Muse"
#    "Needed for the Musepack audio decoder plugin"
#    "http://www.musepack.net/"
#    FALSE "" "")
#endif(K3B_BUILD_MUSE_DECODER_PLUGIN)

#if(K3B_BUILD_SNDFILE_DECODER_PLUGIN)
#  find_package(Sndfile)
#  macro_log_feature( SNDFILE_FOUND "Sndfile"
#    "Needed for the libsndfile audio decoder plugin."
#    "http://www.mega-nerd.com/libsndfile/"
#    FALSE "" "")
#endif(K3B_BUILD_SNDFILE_DECODER_PLUGIN)

#if(K3B_BUILD_LAME_ENCODER_PLUGIN)
#  find_package(Lame)
#  macro_log_feature( LAME_FOUND "Lame mp3 encoder"
#    "Needed for the lame mpf encoder encoder plugin."
#    "http://lame.sourceforge.net/"
#    FALSE "" "")
#endif(K3B_BUILD_LAME_ENCODER_PLUGIN)

#if(K3B_BUILD_OGGVORBIS_DECODER_PLUGIN OR K3B_BUILD_OGGVORBIS_ENCODER_PLUGIN)
#  find_package(OggVorbis)
#  macro_log_feature( OGGVORBIS_FOUND "Ogg Vorbis"
#    "Needed for the K3b Ogg Vorbis decoder and encoder plugins."
#    "http://www.vorbis.com/"
#    FALSE "" "")
#endif(K3B_BUILD_OGGVORBIS_DECODER_PLUGIN OR K3B_BUILD_OGGVORBIS_ENCODER_PLUGIN)


#if (WIN32)
#find_library(SAMPLERATE_LIBRARIES samplerate-0)
#endif(WIN32)

###################  K3b build settings #################################
include(ConfigureChecks)

#macro_bool_to_01(ADD_K3B_DEBUG K3B_DEBUG)

if(K3B_ENABLE_DVD_RIPPING AND DVDREAD_FOUND)
    set(ENABLE_DVD_RIPPING 1)
endif()

if(K3B_ENABLE_TAGLIB AND TAGLIB_FOUND)
    set(ENABLE_TAGLIB 1)
endif()

if(K3B_ENABLE_MUSICBRAINZ AND MUSICBRAINZ_FOUND)
    set(ENABLE_MUSICBRAINZ 1)
endif()

if(K3B_BUILD_FFMPEG_DECODER_PLUGIN AND FFMPEG_FOUND)
    set(BUILD_FFMPEG_DECODER_PLUGIN 1)
endif()

if(K3B_BUILD_FLAC_DECODER_PLUGIN AND FLAC_FOUND AND FLAC++_FOUND)
    set(BUILD_FLAC_DECODER_PLUGIN 1)
endif()

if(K3B_BUILD_MAD_DECODER_PLUGIN AND MAD_FOUND)
    set(BUILD_MAD_DECODER_PLUGIN 1)
endif()

if(K3B_BUILD_MUSE_DECODER_PLUGIN AND MUSE_FOUND)
    set(BUILD_MUSE_DECODER_PLUGIN 1)
endif()

if(K3B_BUILD_SNDFILE_DECODER_PLUGIN AND SNDFILE_FOUND)
    set(BUILD_SNDFILE_DECODER_PLUGIN 1)
endif()

if(K3B_BUILD_LAME_ENCODER_PLUGIN AND LAME_FOUND)
    set(BUILD_LAME_ENCODER_PLUGIN 1)
endif()

if(K3B_BUILD_OGGVORBIS_DECODER_PLUGIN AND OGGVORBIS_FOUND)
    set(BUILD_OGGVORBIS_DECODER_PLUGIN 1)
endif()

if(K3B_BUILD_OGGVORBIS_ENCODER_PLUGIN AND OGGVORBIS_FOUND)
    set(BUILD_OGGVORBIS_ENCODER_PLUGIN 1)
endif()

if(K3B_BUILD_WAVE_DECODER_PLUGIN)
    set(BUILD_WAVE_DECODER_PLUGIN 1)
endif()

if(K3B_BUILD_SOX_ENCODER_PLUGIN)
    set(BUILD_SOX_ENCODER_PLUGIN 1)
endif()

if(K3B_BUILD_EXTERNAL_ENCODER_PLUGIN)
    set(BUILD_EXTERNAL_ENCODER_PLUGIN 1)
endif()

if(K3B_ENABLE_HAL_SUPPORT)
    set(ENABLE_HAL_SUPPORT 1)
endif()

if(QT_QTMULTIMEDIA_FOUND)
    set(ENABLE_AUDIO_PLAYER 1)
endif()

configure_file(config-k3b.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-k3b.h)
include_directories(${CMAKE_BINARY_DIR})

##################  K3b apidox ################################
if(K3B_BUILD_API_DOCS)
    find_package(Doxygen)

    if(DOXYGEN_EXECUTABLE)
        configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.cmake ${CMAKE_BINARY_DIR}/Doxyfile)

        if(EXISTS ${QT_DOC_DIR}/html)
            set(QTDOCS "${QT_DOC_DIR}/html")
        else()
            set(QTDOCS "http://doc.trolltech.com/4.5/")
        endif()

        add_custom_target(apidox
            COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
            COMMAND docs/html/installdox -l qt4.tag@${QTDOCS} docs/html/*.html)
    endif()
endif(K3B_BUILD_API_DOCS)


##################  K3b build environment ################################


add_subdirectory( libk3bdevice )
add_subdirectory( libk3b )
add_subdirectory( src )
add_subdirectory( kioslaves )
add_subdirectory( plugins )
add_subdirectory( doc )
add_subdirectory( tests )


###################  K3b config summary ################################
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
