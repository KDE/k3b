kcoreaddons_add_plugin(k3baudioprojectcddbplugin
    SOURCES k3baudioprojectcddbplugin.cpp
    INSTALL_NAMESPACE "k3b"
    JSON "k3baudioprojectcddbplugin.json")

target_link_libraries(k3baudioprojectcddbplugin
    k3bdevice
    k3blib
    KF5::I18n
    KF5::Cddb
)
