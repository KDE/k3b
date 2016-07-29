set(k3bexternalencoder_PART_SRCS
  k3bexternalencoder.cpp
  k3bexternalencodercommand.cpp )

kde4_add_plugin(k3bexternalencoder
  ${k3bexternalencoder_PART_SRCS})
target_link_libraries(k3bexternalencoder
  ${KDE4_KDECORE_LIBS} k3bdevice
  k3b)

set(k3bexternalencoderkcm_PART_SRCS
  k3bexternalencoderconfigwidget.cpp
  k3bexternalencodercommand.cpp )
kde4_add_ui_files(k3bexternalencoderkcm_PART_SRCS
  base_k3bexternalencoderconfigwidget.ui
  base_k3bexternalencodereditwidget.ui)
kde4_add_plugin(kcm_k3bexternalencoder
  ${k3bexternalencoderkcm_PART_SRCS})
target_link_libraries(kcm_k3bexternalencoder
  ${KDE4_KDEUI_LIBS} k3bdevice
  k3b)

install(TARGETS k3bexternalencoder kcm_k3bexternalencoder
  DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES k3bexternalencoder.desktop kcm_k3bexternalencoder.desktop
  DESTINATION ${SERVICES_INSTALL_DIR} )
