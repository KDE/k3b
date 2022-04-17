kcoreaddons_add_plugin(k3bsoxencoder
    SOURCES k3bsoxencoder.cpp
    INSTALL_NAMESPACE "k3b_plugins")

target_link_libraries(k3bsoxencoder
    k3bdevice
    k3blib
    KF5::I18n
)

ki18n_wrap_ui(ui_sources base_k3bsoxencoderconfigwidget.ui)

kcoreaddons_add_plugin(kcm_k3bsoxencoder INSTALL_NAMESPACE "k3b_plugins/kcms")
target_sources(kcm_k3bsoxencoder PRIVATE k3bsoxencoderconfigwidget.cpp ${ui_sources})

target_link_libraries(kcm_k3bsoxencoder
    k3bdevice
    k3blib
    KF5::I18n
)
