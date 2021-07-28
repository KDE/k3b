kcoreaddons_add_plugin(k3baudiometainforenamerplugin
    SOURCES k3baudiometainforenamerplugin.cpp
    INSTALL_NAMESPACE "k3b")

target_link_libraries(k3baudiometainforenamerplugin
    k3bdevice
    k3blib
    KF5::I18n
    Taglib::Taglib
)
