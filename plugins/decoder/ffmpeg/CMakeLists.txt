


########### next target ###############

find_file(newffmpegavcodecpath avcodec.h /usr/include/libavcodec)

if(newffmpegavcodecpath)
    message(STATUS "found new ffmpegcodecpath")
    add_definitions(-DNEWFFMPEGAVCODECPATH)
else(newffmpegavcodecpath)
    message(STATUS "didn't find new ffmpegcodecpath")
endif(newffmpegavcodecpath)

set(k3bffmpegdecoder_PART_SRCS k3bffmpegdecoder.cpp k3bffmpegwrapper.cpp )

kde4_add_plugin(k3bffmpegdecoder ${k3bffmpegdecoder_PART_SRCS})

target_link_libraries(k3bffmpegdecoder k3bdevice  ${KDE4_KDEUI_LIBS} k3b ${FFMPEG_LIBRARIES} )

install(TARGETS k3bffmpegdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bffmpegdecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

