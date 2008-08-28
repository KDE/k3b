########### next target ###############

set(k3bwavedecoder_PART_SRCS k3bwavedecoder.cpp )

kde4_add_plugin(k3bwavedecoder ${k3bwavedecoder_PART_SRCS})

target_link_libraries(k3bwavedecoder k3bdevice ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3bwavedecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bwavedecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
