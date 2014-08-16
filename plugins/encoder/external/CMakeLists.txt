add_library(k3bexternalencoder MODULE
    k3bexternalencoder.cpp
    k3bexternalencodercommand.cpp
)

target_link_libraries(k3bexternalencoder
    KF5::I18n
    KF5::KDELibs4Support
    k3bdevice
    k3blib
)

qt5_wrap_ui(ui_sources
    base_k3bexternalencoderconfigwidget.ui
    base_k3bexternalencodereditdialog.ui
)

add_library(kcm_k3bexternalencoder MODULE
    k3bexternalencoderconfigwidget.cpp
    k3bexternalencodercommand.cpp
    ${ui_sources}
)

target_link_libraries(kcm_k3bexternalencoder
    KF5::I18n
    KF5::KDELibs4Support
    k3bdevice
    k3blib
)

install(TARGETS k3bexternalencoder kcm_k3bexternalencoder DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES k3bexternalencoder.desktop kcm_k3bexternalencoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
