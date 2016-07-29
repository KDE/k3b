########### next target ###############

set(k3boggvorbisdecoder_PART_SRCS k3boggvorbisdecoder.cpp )

kde4_add_plugin(k3boggvorbisdecoder ${k3boggvorbisdecoder_PART_SRCS})

target_link_libraries(k3boggvorbisdecoder k3bdevice ${KDE4_KDECORE_LIBS}
${OGGVORBIS_LIBRARIES} k3b)

install(TARGETS k3boggvorbisdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3boggvorbisdecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
