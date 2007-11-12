
set(k3bmpcdecoder_PART_SRCS k3bmpcdecoder.cpp k3bmpcwrapper.cpp )

kde4_add_plugin(k3bmpcdecoder WITH_PREFIX ${k3bmpcdecoder_PART_SRCS})

target_link_libraries(k3bmpcdecoder  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3bmpcdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bmpcdecoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )

