########### next target ###############

set(k3blibsndfiledecoder_PART_SRCS k3blibsndfiledecoder.cpp )

kde4_add_plugin(k3blibsndfiledecoder ${k3blibsndfiledecoder_PART_SRCS})

target_link_libraries(k3blibsndfiledecoder k3bdevice k3b ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${SNDFILE_LIBRARIES} )

install(TARGETS k3blibsndfiledecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3blibsndfiledecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

