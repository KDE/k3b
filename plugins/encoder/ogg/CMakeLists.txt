add_library(k3boggvorbisencoder MODULE k3boggvorbisencoder.cpp)

target_link_libraries(k3boggvorbisencoder
    KF5::I18n
    KF5::KDELibs4Support
    k3blib
    k3bdevice
    ${OGGVORBIS_LIBRARIES}
)

qt5_wrap_ui(ui_sources base_k3boggvorbisencodersettingswidget.ui)

add_library(kcm_k3boggvorbisencoder MODULE
    k3boggvorbisencoderconfigwidget.cpp
    ${ui_sources}
)

target_link_libraries(kcm_k3boggvorbisencoder
    KF5::I18n
    KF5::KDELibs4Support
    k3blib
    k3bdevice
    ${OGGVORBIS_LIBRARIES}
)

install(TARGETS k3boggvorbisencoder kcm_k3boggvorbisencoder DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES k3boggvorbisencoder.desktop kcm_k3boggvorbisencoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
