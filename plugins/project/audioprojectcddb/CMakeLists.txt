
set(k3baudioprojectcddbplugin_PART_SRCS
  k3baudioprojectcddbplugin.cpp
  )

kde4_add_plugin(k3baudioprojectcddbplugin ${k3baudioprojectcddbplugin_PART_SRCS})

target_link_libraries(k3baudioprojectcddbplugin
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${KCDDB_LIBRARIES}
  k3b
  k3bdevice
  )

install(TARGETS
  k3baudioprojectcddbplugin
  DESTINATION ${PLUGIN_INSTALL_DIR}
  )
install( FILES
  k3baudioprojectcddbplugin.desktop
  DESTINATION
  ${SERVICES_INSTALL_DIR} )
