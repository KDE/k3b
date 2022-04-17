kcoreaddons_add_plugin(k3boggvorbisencoder
    SOURCES k3boggvorbisencoder.cpp
    INSTALL_NAMESPACE "k3b_plugins")

target_link_libraries(k3boggvorbisencoder
    k3bdevice
    k3blib
    KF5::I18n
    ${OGGVORBIS_LIBRARIES}
)

ki18n_wrap_ui(ui_sources base_k3boggvorbisencodersettingswidget.ui)

kcoreaddons_add_plugin(kcm_k3boggvorbisencoder INSTALL_NAMESPACE "k3b_plugins/kcms")
target_sources(kcm_k3boggvorbisencoder PRIVATE k3boggvorbisencoderconfigwidget.cpp ${ui_sources})

target_link_libraries(kcm_k3boggvorbisencoder
    k3bdevice
    k3blib
    KF5::I18n
    ${OGGVORBIS_LIBRARIES}
)
