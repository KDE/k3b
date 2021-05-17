kcoreaddons_add_plugin(k3boggvorbisdecoder SOURCES k3boggvorbisdecoder.cpp INSTALL_NAMESPACE "k3b_plugins")
kcoreaddons_desktop_to_json(k3boggvorbisdecoder "k3boggvorbisdecoder.desktop" SERVICE_TYPES ${CMAKE_SOURCE_DIR}/libk3b/plugin/k3bplugin.desktop)

target_link_libraries(k3boggvorbisdecoder k3bdevice k3blib KF5::I18n ${OGGVORBIS_LIBRARIES})
