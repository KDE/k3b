set(k3boggvorbisencoder_PART_SRCS
  k3boggvorbisencoder.cpp)
kde4_add_plugin(k3boggvorbisencoder
  ${k3boggvorbisencoder_PART_SRCS})
target_link_libraries(k3boggvorbisencoder
  ${KDE4_KDECORE_LIBS}
  k3b k3bdevice
  ${OGGVORBIS_LIBRARIES})

set(kcm_k3boggvorbisencoder_PART_SRCS
  k3boggvorbisencoderconfigwidget.cpp)
kde4_add_ui_files(kcm_k3boggvorbisencoder_PART_SRCS
  base_k3boggvorbisencodersettingswidget.ui)
kde4_add_plugin(kcm_k3boggvorbisencoder
  ${kcm_k3boggvorbisencoder_PART_SRCS})
target_link_libraries(kcm_k3boggvorbisencoder
  ${KDE4_KDEUI_LIBS}
  k3b k3bdevice
  ${OGGVORBIS_LIBRARIES})

install(TARGETS k3boggvorbisencoder kcm_k3boggvorbisencoder
  DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES k3boggvorbisencoder.desktop kcm_k3boggvorbisencoder.desktop
  DESTINATION ${SERVICES_INSTALL_DIR})
