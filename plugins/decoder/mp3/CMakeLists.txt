


########### next target ###############

set(k3bmaddecoder_PART_SRCS k3bmad.cpp k3bmaddecoder.cpp )

kde4_add_plugin(k3bmaddecoder WITH_PREFIX ${k3bmaddecoder_PART_SRCS})

target_link_libraries(k3bmaddecoder  ${KDE4_KDECORE_LIBS} k3b ${MAD_LIBRARIES} )

install(TARGETS k3bmaddecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bmaddecoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )

