


########### next target ###############

set(k3blibsndfiledecoder_PART_SRCS k3blibsndfiledecoder.cpp )

kde4_add_plugin(k3blibsndfiledecoder WITH_PREFIX ${k3blibsndfiledecoder_PART_SRCS})

target_link_libraries(k3blibsndfiledecoder  ${KDE4_KDECORE_LIBS} k3b ${KDE4_KDEUI_LIBS} sndfile )

install(TARGETS k3blibsndfiledecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3blibsndfiledecoder.desktop DESTINATION
${SERVICES_INSTALL_DIR} )

