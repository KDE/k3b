


########### next target ###############

set(k3boggvorbisencoder_PART_SRCS k3boggvorbisencoder.cpp )

kde4_add_ui3_files(k3boggvorbisencoder_PART_SRCS base_k3boggvorbisencodersettingswidget.ui )

kde4_add_plugin(k3boggvorbisencoder WITH_PREFIX ${k3boggvorbisencoder_PART_SRCS})

target_link_libraries(k3boggvorbisencoder  ${KDE4_KDECORE_LIBS} k3b
${OGGVORBIS_LIBRARIES} )

install(TARGETS k3boggvorbisencoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3boggvorbisencoder.plugin DESTINATION ${DATA_INSTALL_DIR}/k3b/plugins )
