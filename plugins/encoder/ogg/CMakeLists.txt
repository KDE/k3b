kcoreaddons_add_plugin(k3boggvorbisencoder SOURCES k3boggvorbisencoder.cpp INSTALL_NAMESPACE "k3b")
kcoreaddons_desktop_to_json(k3boggvorbisencoder "k3boggvorbisencoder.desktop" SERVICE_TYPES ${CMAKE_SOURCE_DIR}/libk3b/plugin/k3bplugin.desktop)

target_link_libraries(k3boggvorbisencoder
    k3bdevice
    k3blib
    KF5::I18n
    ${OGGVORBIS_LIBRARIES}
)

ki18n_wrap_ui(ui_sources base_k3boggvorbisencodersettingswidget.ui)

add_library(kcm_k3boggvorbisencoder MODULE
    k3boggvorbisencoderconfigwidget.cpp
    ${ui_sources}
)

target_link_libraries(kcm_k3boggvorbisencoder
    k3bdevice
    k3blib
    KF5::I18n
    ${OGGVORBIS_LIBRARIES}
)

install(TARGETS kcm_k3boggvorbisencoder DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES kcm_k3boggvorbisencoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
