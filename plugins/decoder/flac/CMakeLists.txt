


########### next target ###############

set(k3bflacdecoder_PART_SRCS k3bflacdecoder.cpp )

kde4_add_plugin(k3bflacdecoder WITH_PREFIX ${k3bflacdecoder_PART_SRCS})

target_link_libraries(k3bflacdecoder  k3b ${KDE4_KDEUI_LIBS} FLAC++ FLAC
${FLAC_LIBRARIES} )

install(TARGETS k3bflacdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bflacdecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

