kcoreaddons_add_plugin(k3baudioprojectcddbplugin SOURCES k3baudioprojectcddbplugin.cpp INSTALL_NAMESPACE "k3b")
kcoreaddons_desktop_to_json(k3baudioprojectcddbplugin "k3baudioprojectcddbplugin.desktop" SERVICE_TYPES ${CMAKE_SOURCE_DIR}/libk3b/plugin/k3bplugin.desktop)

target_link_libraries(k3baudioprojectcddbplugin
    k3bdevice
    k3blib
    KF5::I18n
    KF5::Cddb
)
