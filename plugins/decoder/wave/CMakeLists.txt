kcoreaddons_add_plugin(k3bwavedecoder SOURCES k3bwavedecoder.cpp INSTALL_NAMESPACE "k3b_plugins")
kcoreaddons_desktop_to_json(k3bwavedecoder "k3bwavedecoder.desktop" SERVICE_TYPES ${CMAKE_SOURCE_DIR}/libk3b/plugin/k3bplugin.desktop)

target_link_libraries(k3bwavedecoder k3bdevice k3blib KF5::I18n)
