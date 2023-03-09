kcoreaddons_add_plugin(k3bmaddecoder
    SOURCES k3bmad.cpp k3bmaddecoder.cpp
    INSTALL_NAMESPACE "k3b_plugins")

target_include_directories(k3bmaddecoder PRIVATE ${MAD_INCLUDE_DIR})

target_link_libraries(k3bmaddecoder k3bdevice k3blib KF${KF_MAJOR_VERSION}::I18n ${MAD_LIBRARIES})

if(ENABLE_TAGLIB)
    target_link_libraries(k3bmaddecoder Taglib::Taglib)
endif()
