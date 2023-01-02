cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "22")
set (RELEASE_SERVICE_VERSION_MINOR "12")
set (RELEASE_SERVICE_VERSION_MICRO "1")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(k3b VERSION ${RELEASE_SERVICE_VERSION})

set (QT_MIN_VERSION "5.14.0")
set (KF5_MIN_VERSION "5.88.0")
set (KDE_COMPILERSETTINGS_LEVEL "5.84")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" "${ECM_MODULE_PATH}")

##################  K3b version  ################################
set(K3B_VERSION_STRING ${RELEASE_SERVICE_VERSION})

# increase on BC breakage
set(K3B_LIB_VERSION_MAJOR 8)

# increase after adding functionality
set(K3B_LIB_VERSION_MINOR 0)
set(K3B_LIB_VERSION_RELEASE 0)


##################  K3b options #################################
option(K3B_DEBUG "K3b additional debugging support" OFF)
if(K3B_DEBUG)
    add_definitions(-g)
endif()
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
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(FeatureSummary)
include(GenerateExportHeader)
include(ECMInstallIcons)
include(ECMEnableSanitizers)
include(ECMSetupQtPluginMacroNames)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus Gui)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Archive Auth Config CoreAddons DocTools FileMetaData I18n IconThemes JobWidgets
                                     KCMUtils KIO Notifications NewStuff NotifyConfig Service Solid WidgetsAddons XmlGui)

find_package(Qt5WebKitWidgets)
set_package_properties(Qt5WebKitWidgets PROPERTIES
    DESCRIPTION "Qt5WebKitWidgets is used for prettier k3bdiskinfoview."
    TYPE OPTIONAL)
if(Qt5WebKitWidgets_FOUND)
    add_definitions(-DHAVE_QT5WEBKITWIDGETS)
endif()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

option(WITH_NEW_SOLID_JOB "WIP: base Job class" Off)
add_feature_info(Solid::Job WITH_NEW_SOLID_JOB "WIP: Base class for Solid Asynchronous apis")
if(WITH_NEW_SOLID_JOB)
    add_definitions(-DWITH_NEW_SOLID_JOB)
endif()

find_package(Iconv)
set(HAVE_ICONV "${ICONV_FOUND}")

find_package(Samplerate)
set_package_properties(Samplerate PROPERTIES
    DESCRIPTION "Audio sampling rate conversion."
    URL "http://www.mega-nerd.com/SRC"
    TYPE REQUIRED)

find_package(KF5Cddb 5.1 REQUIRED)
set_package_properties(KF5Cddb PROPERTIES
    DESCRIPTION "KF5 branch for CDDB library"
    URL "https://commits.kde.org/libkcddb"
    TYPE REQUIRED
    PURPOSE "libkcddb is used to retrieve audio CD meta data from the internet.")

if(K3B_ENABLE_DVD_RIPPING)
    find_package(DvdRead)
    set_package_properties(DvdRead PROPERTIES
        DESCRIPTION "Libdvdread provides a simple foundation for reading DVD video disks."
        URL "https://www.videolan.org/developers/libdvdnav.html"
        TYPE OPTIONAL)

    set(ENABLE_DVD_RIPPING "${DVDREAD_FOUND}")
endif()

if(K3B_ENABLE_TAGLIB)
    find_package(Taglib)
    set_package_properties(Taglib PROPERTIES
        DESCRIPTION "Read and write tags in audio files"
        URL "https://taglib.org/"
        TYPE OPTIONAL)

    set(ENABLE_TAGLIB "${TAGLIB_FOUND}")
endif()

if(K3B_ENABLE_MUSICBRAINZ)
    find_package(MusicBrainz)
    set_package_properties(MusicBrainz PROPERTIES
        DESCRIPTION "Provide information about the CD, about the artist or about related information"
        URL "https://musicbrainz.org/"
        TYPE OPTIONAL)

    set(ENABLE_MUSICBRAINZ "${MUSICBRAINZ_FOUND}")
endif()

if(K3B_BUILD_FFMPEG_DECODER_PLUGIN)
    find_package(FFmpeg 4.4.0)
    set_package_properties(FFmpeg PROPERTIES
        PURPOSE "Needed for the K3b FFmpeg decoder plugin which can decode virtually all audio types."
        URL "https://ffmpeg.org/"
        TYPE OPTIONAL)

    if(FFMPEG_FOUND)
        include(CheckSymbolExists)
        include(CMakePushCheckState)
        cmake_push_check_state()

        if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
            # FindKDE4Internal.cmake screws things up
            set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} -std=c99)
        endif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")

        if(FFMPEG_INCLUDE_DIR_OLD_STYLE)
            set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${FFMPEG_INCLUDE_DIR_OLD_STYLE})
            set(FFMPEG_HEADERS ffmpeg/avcodec.h ffmpeg/avformat.h)
        else(FFMPEG_INCLUDE_DIR_OLD_STYLE)
            set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${FFMPEG_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIRS})
            set(FFMPEG_HEADERS libavcodec/avcodec.h libavformat/avformat.h)
            set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -DNEWFFMPEGAVCODECPATH)
        endif(FFMPEG_INCLUDE_DIR_OLD_STYLE)
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${FFMPEG_LIBRARIES})

        check_symbol_exists(avformat_open_input "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVFORMAT_OPEN_INPUT)
        check_symbol_exists(av_dump_format "${FFMPEG_HEADERS}" HAVE_FFMPEG_AV_DUMP_FORMAT)
        check_symbol_exists(avformat_find_stream_info "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVFORMAT_FIND_STREAM_INFO)
        check_symbol_exists(avformat_close_input "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVFORMAT_CLOSE_INPUT)
        check_symbol_exists(avcodec_open2 "${FFMPEG_HEADERS}" HAVE_FFMPEG_AVCODEC_OPEN2)
        include(CheckCSourceCompiles)
        check_c_source_compiles("
            #ifdef NEWFFMPEGAVCODECPATH
            #include <libavcodec/avcodec.h>
            #else
            #include <ffmpeg/avcodec.h>
            #endif
            int main() { enum AVMediaType t = AVMEDIA_TYPE_UNKNOWN; return 0; }
            " HAVE_FFMPEG_AVMEDIA_TYPE)
        check_c_source_compiles("
            #ifdef NEWFFMPEGAVCODECPATH
            #include <libavcodec/avcodec.h>
            #else
            #include <ffmpeg/avcodec.h>
            #endif
            int main() {
            #if LIBAVCODEC_BUILD >= AV_VERSION_INT(54,25,0)
            # Not needed here. AV_CODEC_ID_MP3 exists.
            #else
            enum CodecID t = CODEC_ID_MP3;
            #endif
            return 0;
            }
            " HAVE_FFMPEG_CODEC_MP3)

        cmake_pop_check_state()
    endif(FFMPEG_FOUND)

    set(BUILD_FFMPEG_DECODER_PLUGIN "${FFMPEG_FOUND}")
endif(K3B_BUILD_FFMPEG_DECODER_PLUGIN)

if(K3B_BUILD_FLAC_DECODER_PLUGIN)
    find_package(Flac)
    set_package_properties(Flac PROPERTIES
        PURPOSE "Needed for the Flac audio decoder plugin."
        URL "https://xiph.org/flac/"
        TYPE OPTIONAL)

    find_package(Flac++)
    set_package_properties(Flac++ PROPERTIES
        PURPOSE "Needed for the Flac audio decoder plugin."
        URL "https://xiph.org/flac/"
        TYPE OPTIONAL)

    if(FLAC_FOUND AND FLAC++_FOUND)
        set(BUILD_FLAC_DECODER_PLUGIN 1)
    endif()
endif()

if(K3B_BUILD_MAD_DECODER_PLUGIN)
    find_package(Mad)
    set_package_properties(Mad PROPERTIES
        PURPOSE "Needed for the mp3 audio decoder plugin."
        URL "https://www.underbit.com/products/mad"
        TYPE OPTIONAL)

    set(BUILD_MAD_DECODER_PLUGIN "${MAD_FOUND}")
endif()

if(K3B_BUILD_MUSE_DECODER_PLUGIN)
    find_package(Muse)
    set_package_properties(Muse PROPERTIES
        PURPOSE "Needed for the Musepack audio decoder plugin"
        URL "https://www.musepack.net/"
        TYPE OPTIONAL)

    set(BUILD_MUSE_DECODER_PLUGIN "${Muse_FOUND}")
endif()

if(K3B_BUILD_SNDFILE_DECODER_PLUGIN)
    find_package(Sndfile)
    set_package_properties(Sndfile PROPERTIES
        PURPOSE "Needed for the libsndfile audio decoder plugin."
        URL "http://www.mega-nerd.com/libsndfile"
        TYPE OPTIONAL)

    set(BUILD_SNDFILE_DECODER_PLUGIN "${SNDFILE_FOUND}")
endif()

if(K3B_BUILD_LAME_ENCODER_PLUGIN)
    find_package(Lame)
    set_package_properties(Lame PROPERTIES
        DESCRIPTION "Lame mp3 encoder"
        PURPOSE "Needed for the lame mpf encoder encoder plugin."
        URL "https://lame.sourceforge.io/"
        TYPE OPTIONAL)

    set(BUILD_LAME_ENCODER_PLUGIN "${LAME_FOUND}")
endif()

if(K3B_BUILD_OGGVORBIS_DECODER_PLUGIN OR K3B_BUILD_OGGVORBIS_ENCODER_PLUGIN)
    find_package(OggVorbis)
    set_package_properties(OggVorbis PROPERTIES
        DESCRIPTION "Ogg Vorbis"
        PURPOSE "Needed for the K3b Ogg Vorbis decoder and encoder plugins."
        URL "https://xiph.org/vorbis/"
        TYPE OPTIONAL)

    set(BUILD_OGGVORBIS_DECODER_PLUGIN "${OGGVORBIS_FOUND}")
    set(BUILD_OGGVORBIS_ENCODER_PLUGIN "${OGGVORBIS_FOUND}")
endif()

###################  K3b build settings #################################
include(ConfigureChecks)

set(BUILD_WAVE_DECODER_PLUGIN "${K3B_BUILD_WAVE_DECODER_PLUGIN}")
set(BUILD_SOX_ENCODER_PLUGIN "${K3B_BUILD_SOX_ENCODER_PLUGIN}")
set(BUILD_EXTERNAL_ENCODER_PLUGIN "${K3B_BUILD_EXTERNAL_ENCODER_PLUGIN}")

set(ENABLE_HAL_SUPPORT "${K3B_ENABLE_HAL_SUPPORT}")
set(ENABLE_AUDIO_PLAYER "${QT_QTMULTIMEDIA_FOUND}")

if(IS_ABSOLUTE ${KDE_INSTALL_DATADIR})
  set(K3B_DATA_INSTALL_DIR ${KDE_INSTALL_DATADIR}/k3b)
else()
  set(K3B_DATA_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${KDE_INSTALL_DATADIR}/k3b)
endif()

configure_file(config-k3b.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-k3b.h)
include_directories(${CMAKE_BINARY_DIR})

ecm_setup_qtplugin_macro_names(
    JSON_NONE
        K3B_EXPORT_PLUGIN
        K3B_EXPORT_PLUGIN_CONFIG_WIDGET
)

##################  K3b apidox ################################
if(K3B_BUILD_API_DOCS)
    find_package(Doxygen)

    if(DOXYGEN_EXECUTABLE)
        configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.cmake ${CMAKE_BINARY_DIR}/Doxyfile)

        if(EXISTS ${QT_DOC_DIR}/html)
            set(QTDOCS "${QT_DOC_DIR}/html")
        else()
            set(QTDOCS "https://doc.qt.io/qt-5/")
        endif()

        add_custom_target(apidox
            COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
            COMMAND docs/html/installdox -l qt5.tag@${QTDOCS} docs/html/*.html)
    endif()
endif(K3B_BUILD_API_DOCS)

##################  K3b build environment ################################
add_subdirectory( libk3bdevice )
add_subdirectory( libk3b )
add_subdirectory( src )
add_subdirectory( kioslaves )
add_subdirectory( plugins )
add_subdirectory( doc )
if(BUILD_TESTING)
    find_package(Qt5Test REQUIRED)
    find_package(LibFuzzer)
    set_package_properties(LibFuzzer PROPERTIES
        PURPOSE "libFuzzer – a library for coverage-guided fuzz testing."
        URL "https://llvm.org/docs/LibFuzzer.html"
        TYPE OPTIONAL)
    add_subdirectory(tests)
endif()

ki18n_install(po)
kdoctools_install(po)

###################  K3b config summary ################################
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
