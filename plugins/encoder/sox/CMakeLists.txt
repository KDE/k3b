kcoreaddons_add_plugin(k3bsoxencoder
    SOURCES k3bsoxencoder.cpp
    INSTALL_NAMESPACE "k3b")

target_link_libraries(k3bsoxencoder
    k3bdevice
    k3blib
    KF5::I18n
)

ki18n_wrap_ui(ui_sources base_k3bsoxencoderconfigwidget.ui)

add_library(kcm_k3bsoxencoder MODULE k3bsoxencoderconfigwidget.cpp ${ui_sources})

target_link_libraries(kcm_k3bsoxencoder
    k3bdevice
    k3blib
    KF5::I18n
)

install(TARGETS kcm_k3bsoxencoder DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES kcm_k3bsoxencoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
