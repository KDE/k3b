add_library(k3baudioprojectcddbplugin MODULE k3baudioprojectcddbplugin.cpp)

target_link_libraries(k3baudioprojectcddbplugin
    Qt5::Widgets
    KF5::I18n
    KF5::KIOCore
    KF5::WidgetsAddons
    k3blib
    k3bdevice
    KDE::Libkcddb::kcddb
)

install(TARGETS k3baudioprojectcddbplugin DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES k3baudioprojectcddbplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
