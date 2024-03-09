kcoreaddons_add_plugin(k3blameencoder
    SOURCES k3blameencoder.cpp
    INSTALL_NAMESPACE "k3b_plugins")

target_link_libraries(k3blameencoder
    k3bdevice
    k3blib
    KF${KF_MAJOR_VERSION}::I18n
    ${LAME_LIBRARIES}
)

ki18n_wrap_ui(ui_sources
    base_k3blameencodersettingswidget.ui
    base_k3bmanualbitratesettingsdialog.ui
)

kcoreaddons_add_plugin(kcm_k3blameencoder INSTALL_NAMESPACE "k3b_plugins/kcms/")
target_sources(kcm_k3blameencoder PRIVATE
    k3blameencoderconfigwidget.cpp
    k3blamemanualsettingsdialog.cpp
    ${ui_sources}
)

target_link_libraries(kcm_k3blameencoder
    k3bdevice
    k3blib
    KF${KF_MAJOR_VERSION}::I18n
)
