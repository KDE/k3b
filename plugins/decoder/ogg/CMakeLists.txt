kcoreaddons_add_plugin(k3boggvorbisdecoder
    SOURCES k3boggvorbisdecoder.cpp
    INSTALL_NAMESPACE "k3b"
    JSON "k3boggvorbisdecoder.json")

target_link_libraries(k3boggvorbisdecoder k3bdevice k3blib KF5::I18n ${OGGVORBIS_LIBRARIES})
