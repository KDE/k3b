########### next target ###############

set(k3bwavedecoder_PART_SRCS k3bwavedecoder.cpp )

kde4_add_plugin(k3bwavedecoder WITH_PREFIX ${k3bwavedecoder_PART_SRCS})

target_link_libraries(k3bwavedecoder  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3bwavedecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bwavedecoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )
