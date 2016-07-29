
########### next target ###############

set(kio_videodvd_PART_SRCS videodvd.cpp )

kde4_add_plugin(kio_videodvd ${kio_videodvd_PART_SRCS})

target_link_libraries(kio_videodvd ${KDE4_KIO_LIBS} k3b k3bdevice )

install(TARGETS kio_videodvd DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  videodvd.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES  videodvd.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/services )

