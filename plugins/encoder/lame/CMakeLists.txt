# KCM
set(k3blameencoderconfig_PART_SRCS
  k3blameencoderconfigwidget.cpp
  k3blamemanualsettingsdialog.cpp)

kde4_add_ui_files(k3blameencoderconfig_PART_SRCS
  base_k3blameencodersettingswidget.ui
  base_k3bmanualbitratesettingswidget.ui )

kde4_add_plugin(kcm_k3blameencoder
  ${k3blameencoderconfig_PART_SRCS})

target_link_libraries(kcm_k3blameencoder
  k3bdevice
  k3b
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${QT_QTGUI_LIBRARY})


# Plugin
kde4_add_plugin(k3blameencoder
  k3blameencoder.cpp)

target_link_libraries(k3blameencoder
  ${KDE4_KDECORE_LIBS}
  k3b k3bdevice
  mp3lame)

install(TARGETS
  k3blameencoder kcm_k3blameencoder
  DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES
  k3blameencoder.desktop kcm_k3blameencoder.desktop
  DESTINATION ${SERVICES_INSTALL_DIR} )

