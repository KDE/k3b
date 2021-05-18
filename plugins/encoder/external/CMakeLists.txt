kcoreaddons_add_plugin(k3bexternalencoder SOURCES k3bexternalencoder.cpp k3bexternalencodercommand.cpp INSTALL_NAMESPACE "k3b")
kcoreaddons_desktop_to_json(k3bexternalencoder "k3bexternalencoder.desktop" SERVICE_TYPES ${CMAKE_SOURCE_DIR}/libk3b/plugin/k3bplugin.desktop)

target_link_libraries(k3bexternalencoder
    k3bdevice
    k3blib
    KF5::I18n
)

ki18n_wrap_ui(ui_sources
    base_k3bexternalencoderconfigwidget.ui
    base_k3bexternalencodereditdialog.ui
)

add_library(kcm_k3bexternalencoder MODULE
    k3bexternalencoderconfigwidget.cpp
    k3bexternalencodercommand.cpp
    ${ui_sources}
)

target_link_libraries(kcm_k3bexternalencoder
    k3bdevice
    k3blib
    KF5::I18n
)

install(TARGETS kcm_k3bexternalencoder DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kcm_k3bexternalencoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
