


########### next target ###############

if(FFMPEG_INCLUDE_DIR_OLD_STYLE)
    message(STATUS "didn't find new ffmpegcodecpath")
    include_directories (${FFMPEG_INCLUDE_DIR_OLD_STYLE})
else(FFMPEG_INCLUDE_DIR_OLD_STYLE)
    message(STATUS "found new ffmpegcodecpath")
    add_definitions(-DNEWFFMPEGAVCODECPATH)
    include_directories (${FFMPEG_INCLUDE_DIR})
endif(FFMPEG_INCLUDE_DIR_OLD_STYLE)

set(k3bffmpegdecoder_PART_SRCS k3bffmpegdecoder.cpp k3bffmpegwrapper.cpp )

kde4_add_plugin(k3bffmpegdecoder ${k3bffmpegdecoder_PART_SRCS})

target_link_libraries(k3bffmpegdecoder k3bdevice  ${KDE4_KDEUI_LIBS} k3b ${FFMPEG_LIBRARIES} )

install(TARGETS k3bffmpegdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bffmpegdecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

