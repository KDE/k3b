########### next target ###############

set(k3boggvorbisdecoder_PART_SRCS k3boggvorbisdecoder.cpp )

kde4_add_plugin(k3boggvorbisdecoder WITH_PREFIX ${k3boggvorbisdecoder_PART_SRCS})

target_link_libraries(k3boggvorbisdecoder  ${KDE4_KDECORE_LIBS}
${OGGVORBIS_LIBRARIES} k3b)

install(TARGETS k3boggvorbisdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3boggvorbisdecoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )
