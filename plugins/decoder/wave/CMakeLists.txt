kcoreaddons_add_plugin(k3bwavedecoder
    SOURCES k3bwavedecoder.cpp
    INSTALL_NAMESPACE "k3b")

target_link_libraries(k3bwavedecoder k3bdevice k3blib KF5::I18n)
