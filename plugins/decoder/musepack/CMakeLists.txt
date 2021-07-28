
configure_file(k3bmpc_config.h.cmake k3bmpc_config.h)

kcoreaddons_add_plugin(k3bmpcdecoder
    SOURCES k3bmpcdecoder.cpp k3bmpcwrapper.cpp
    INSTALL_NAMESPACE "k3b")

target_include_directories(k3bmpcdecoder PRIVATE ${MUSE_INCLUDE_DIR})

target_link_libraries(k3bmpcdecoder k3bdevice k3blib KF5::I18n ${MUSE_LIBRARIES})
