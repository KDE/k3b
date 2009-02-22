
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${MUSE_INCLUDE_DIR}
)

set(k3bmpcdecoder_PART_SRCS k3bmpcdecoder.cpp k3bmpcwrapper.cpp )

configure_file(k3bmpc_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/k3bmpc_config.h)

kde4_add_plugin(k3bmpcdecoder ${k3bmpcdecoder_PART_SRCS})

target_link_libraries(k3bmpcdecoder k3bdevice  ${KDE4_KDECORE_LIBS} k3b
${MUSE_LIBRARIES})

install(TARGETS k3bmpcdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES  k3bmpcdecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

