kcoreaddons_add_plugin(k3bffmpegdecoder SOURCES k3bffmpegdecoder.cpp k3bffmpegwrapper.cpp INSTALL_NAMESPACE "k3b_plugins")
kcoreaddons_desktop_to_json(k3bffmpegdecoder "k3bffmpegdecoder.desktop" SERVICE_TYPES ${CMAKE_SOURCE_DIR}/libk3b/plugin/k3bplugin.desktop)

if(FFMPEG_INCLUDE_DIR_OLD_STYLE)
    message(STATUS "didn't find new ffmpegcodecpath")
    target_include_directories(k3bffmpegdecoder PRIVATE ${FFMPEG_INCLUDE_DIR_OLD_STYLE})
else()
    message(STATUS "found new ffmpegcodecpath")
    target_compile_definitions(k3bffmpegdecoder PRIVATE NEWFFMPEGAVCODECPATH)
    target_include_directories(k3bffmpegdecoder PRIVATE ${FFMPEG_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIRS})
endif()

target_link_libraries(k3bffmpegdecoder k3bdevice k3blib KF5::I18n ${FFMPEG_LIBRARIES})
