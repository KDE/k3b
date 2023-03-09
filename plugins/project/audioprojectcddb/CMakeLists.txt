kcoreaddons_add_plugin(k3baudioprojectcddbplugin
    SOURCES k3baudioprojectcddbplugin.cpp
    INSTALL_NAMESPACE "k3b_plugins")

target_link_libraries(k3baudioprojectcddbplugin
    k3bdevice
    k3blib
    KF${KF_MAJOR_VERSION}::I18n
    ${KCDDB_TARGET_NAME}
)
