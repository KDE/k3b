kcoreaddons_add_plugin(k3blameencoder
    SOURCES k3blameencoder.cpp
    INSTALL_NAMESPACE "k3b"
    JSON "k3blameencoder.json")

target_link_libraries(k3blameencoder
    k3bdevice
    k3blib
    KF5::I18n
    ${LAME_LIBRARIES}
)

ki18n_wrap_ui(ui_sources
    base_k3blameencodersettingswidget.ui
    base_k3bmanualbitratesettingsdialog.ui
)

add_library(kcm_k3blameencoder MODULE
    k3blameencoderconfigwidget.cpp
    k3blamemanualsettingsdialog.cpp
    ${ui_sources}
)

target_link_libraries(kcm_k3blameencoder
    k3bdevice
    k3blib
    KF5::I18n
)

install(TARGETS kcm_k3blameencoder DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kcm_k3blameencoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
