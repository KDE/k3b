
########### next target ###############

set(k3baudiometainforenamerplugin_PART_SRCS k3baudiometainforenamerplugin.cpp )

kde4_add_plugin(k3baudiometainforenamerplugin WITH_PREFIX ${k3baudiometainforenamerplugin_PART_SRCS})

target_link_libraries(k3baudiometainforenamerplugin  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3baudiometainforenamerplugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3baudiometainforenamerplugin.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )

