kcoreaddons_add_plugin(k3bwavedecoder
    SOURCES k3bwavedecoder.cpp
    INSTALL_NAMESPACE "k3b_plugins")

target_link_libraries(k3bwavedecoder k3bdevice k3blib KF6::I18n)
