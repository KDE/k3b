
set(k3baudioprojectcddbplugin_PART_SRCS k3baudioprojectcddbplugin.cpp )

kde4_add_plugin(k3baudioprojectcddbplugin WITH_PREFIX ${k3baudioprojectcddbplugin_PART_SRCS})

target_link_libraries(k3baudioprojectcddbplugin  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3baudioprojectcddbplugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3baudioprojectcddbplugin.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )


