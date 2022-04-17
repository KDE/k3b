kcoreaddons_add_plugin(k3bexternalencoder
    SOURCES k3bexternalencoder.cpp k3bexternalencodercommand.cpp
    INSTALL_NAMESPACE "k3b_plugins")

target_link_libraries(k3bexternalencoder
    k3bdevice
    k3blib
    KF5::I18n
)

ki18n_wrap_ui(ui_sources
    base_k3bexternalencoderconfigwidget.ui
    base_k3bexternalencodereditdialog.ui
)

kcoreaddons_add_plugin(kcm_k3bexternalencoder INSTALL_NAMESPACE "k3b_plugins/kcms")
target_sources(kcm_k3bexternalencoder PRIVATE
    k3bexternalencoderconfigwidget.cpp
    k3bexternalencodercommand.cpp
    ${ui_sources}
)

target_link_libraries(kcm_k3bexternalencoder
    k3bdevice
    k3blib
    KF5::I18n
)
