
if(FFMPEG_INCLUDE_DIR_OLD_STYLE)
    message(STATUS "didn't find new ffmpegcodecpath")
    include_directories (${FFMPEG_INCLUDE_DIR_OLD_STYLE})
else()
    message(STATUS "found new ffmpegcodecpath")
    add_definitions(-DNEWFFMPEGAVCODECPATH)
    include_directories (${FFMPEG_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIRS})
endif()

add_library(k3bffmpegdecoder MODULE k3bffmpegdecoder.cpp k3bffmpegwrapper.cpp)

target_link_libraries(k3bffmpegdecoder k3bdevice k3blib KF5::KDELibs4Support KF5::I18n ${FFMPEG_LIBRARIES})

install(TARGETS k3bffmpegdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES k3bffmpegdecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
