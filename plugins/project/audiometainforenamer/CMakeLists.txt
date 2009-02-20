include_directories(
  ${TAGLIB_INCLUDES}
)

set(k3baudiometainforenamerplugin_PART_SRCS
  k3baudiometainforenamerplugin.cpp
  )

kde4_add_plugin(k3baudiometainforenamerplugin
  ${k3baudiometainforenamerplugin_PART_SRCS})

target_link_libraries(k3baudiometainforenamerplugin
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${TAGLIB_LIBRARIES}
  ${KDE4_KDE3SUPPORT_LIBS}
  k3b )

install(TARGETS k3baudiometainforenamerplugin DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES  k3baudiometainforenamerplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

