kcoreaddons_add_plugin(k3bmaddecoder
    SOURCES k3bmad.cpp k3bmaddecoder.cpp
    INSTALL_NAMESPACE "k3b"
    JSON "k3bmaddecoder.json")

target_include_directories(k3bmaddecoder PRIVATE ${MAD_INCLUDE_DIR})

target_link_libraries(k3bmaddecoder k3bdevice k3blib KF5::I18n ${MAD_LIBRARIES})

if(ENABLE_TAGLIB)
    target_link_libraries(k3bmaddecoder Taglib::Taglib)
endif()
