kcoreaddons_add_plugin(k3bmaddecoder SOURCES k3bmad.cpp k3bmaddecoder.cpp INSTALL_NAMESPACE "k3b_plugins")
kcoreaddons_desktop_to_json(k3bmaddecoder "k3bmaddecoder.desktop" SERVICE_TYPES ${CMAKE_SOURCE_DIR}/libk3b/plugin/k3bplugin.desktop)

target_include_directories(k3bmaddecoder PRIVATE ${MAD_INCLUDE_DIR} ${TAGLIB_INCLUDES})

target_link_libraries(k3bmaddecoder k3bdevice k3blib KF5::I18n ${MAD_LIBRARIES})

if(ENABLE_TAGLIB)
    target_link_libraries(k3bmaddecoder ${TAGLIB_LIBRARIES})
endif()
