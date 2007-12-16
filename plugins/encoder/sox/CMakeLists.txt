


########### next target ###############

set(k3bsoxencoder_PART_SRCS k3bsoxencoder.cpp )

kde4_add_ui_files(k3bsoxencoder_PART_SRCS base_k3bsoxencoderconfigwidget.ui )

kde4_add_plugin(k3bsoxencoder WITH_PREFIX ${k3bsoxencoder_PART_SRCS})

target_link_libraries(k3bsoxencoder  ${KDE4_KDECORE_LIBS} k3b )

install(TARGETS k3bsoxencoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bsoxencoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})

