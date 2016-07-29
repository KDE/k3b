


########### next target ###############

set(k3bmaddecoder_PART_SRCS k3bmad.cpp k3bmaddecoder.cpp )

kde4_add_plugin(k3bmaddecoder 
  ${k3bmaddecoder_PART_SRCS})

target_link_libraries(k3bmaddecoder
  k3b k3bdevice
  ${KDE4_KDECORE_LIBS}
  ${MAD_LIBRARIES} )

if(ENABLE_TAGLIB)
  target_link_libraries(k3bmaddecoder
    ${TAGLIB_LIBRARIES})
endif(ENABLE_TAGLIB)

install(TARGETS k3bmaddecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bmaddecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

