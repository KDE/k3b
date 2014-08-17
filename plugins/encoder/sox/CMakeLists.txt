add_library(k3bsoxencoder MODULE k3bsoxencoder.cpp)

target_link_libraries(k3bsoxencoder
    k3bdevice
    k3blib
    KF5::I18n
)

qt5_wrap_ui(ui_sources base_k3bsoxencoderconfigwidget.ui)

add_library(kcm_k3bsoxencoder MODULE k3bsoxencoderconfigwidget.cpp ${ui_sources})

target_link_libraries(kcm_k3bsoxencoder
    k3bdevice
    k3blib
    KF5::I18n
)

install(TARGETS k3bsoxencoder kcm_k3bsoxencoder DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES k3bsoxencoder.desktop kcm_k3bsoxencoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
