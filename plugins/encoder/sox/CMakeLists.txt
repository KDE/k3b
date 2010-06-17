#KCM
set(k3bsoxencoderconfig_PART_SRCS
    k3bsoxencoderconfigwidget.cpp)

kde4_add_ui_files(k3bsoxencoderconfig_PART_SRCS
    base_k3bsoxencoderconfigwidget.ui)

kde4_add_plugin(kcm_k3bsoxencoder
  ${k3bsoxencoderconfig_PART_SRCS})

target_link_libraries(kcm_k3bsoxencoder
  k3bdevice
  k3b
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${QT_QTGUI_LIBRARY})

#Plugin
kde4_add_plugin(k3bsoxencoder
  k3bsoxencoder.cpp)

target_link_libraries(k3bsoxencoder
    k3bdevice
    k3b
    ${KDE4_KDEUI_LIBS}
    ${QT_QTGUI_LIBRARY} )

install(TARGETS
    k3bsoxencoder kcm_k3bsoxencoder
    DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES
    k3bsoxencoder.desktop kcm_k3bsoxencoder.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})

