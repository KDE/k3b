add_subdirectory(worker)

########### next target ###############

include_directories(
    ../libk3b/core/
    ${POLKITQT_INCLUDE_DIR} )

set(kcm_k3bsetup_PART_SRCS
    k3bsetup.cpp
    k3bsetupdevicesmodel.cpp
    k3bsetupprogramitem.cpp
    k3bsetupprogramsmodel.cpp )

kde4_add_ui_files(kcm_k3bsetup_PART_SRCS base_k3bsetup.ui )

kde4_add_plugin(kcm_k3bsetup ${kcm_k3bsetup_PART_SRCS})

target_link_libraries(kcm_k3bsetup ${KDE4_KDEUI_LIBS} ${POLKITQT_LIBRARIES} k3b k3bdevice )

install(TARGETS kcm_k3bsetup DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( PROGRAMS  k3bsetup DESTINATION ${BIN_INSTALL_DIR} )
install( FILES  k3bsetup.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
