
configure_file(k3bmpc_config.h.cmake k3bmpc_config.h)

add_library(k3bmpcdecoder MODULE k3bmpcdecoder.cpp k3bmpcwrapper.cpp)

target_include_directories(k3bmpcdecoder PRIVATE ${MUSE_INCLUDE_DIR})

target_link_libraries(k3bmpcdecoder k3bdevice k3blib KF5::I18n ${MUSE_LIBRARIES})

install(TARGETS k3bmpcdecoder DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES  k3bmpcdecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
