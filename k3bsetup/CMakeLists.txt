########### next target ###############

include_directories(
    ../libk3b/core/
)

set(kcm_k3bsetup_PART_SRCS
    k3bsetup.cpp
    k3bsetupdevicesmodel.cpp
    k3bsetupprogramitem.cpp
    k3bsetupprogramsmodel.cpp )

kde4_add_ui_files(kcm_k3bsetup_PART_SRCS base_k3bsetup.ui )

kde4_add_plugin(kcm_k3bsetup ${kcm_k3bsetup_PART_SRCS})

target_link_libraries(kcm_k3bsetup ${KDE4_KDEUI_LIBS} k3b k3bdevice )

install(TARGETS kcm_k3bsetup DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( PROGRAMS  k3bsetup DESTINATION ${BIN_INSTALL_DIR} )
install( FILES  k3bsetup.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


########### next target ###############
kde4_add_executable(k3bsetuphelper k3bsetupworker.cpp k3bsetupprogramitem.cpp)
target_link_libraries(k3bsetuphelper ${KDE4_KDECORE_LIBRARY})
install(TARGETS k3bsetuphelper DESTINATION ${LIBEXEC_INSTALL_DIR})

kde4_install_auth_helper_files(k3bsetuphelper org.kde.kcontrol.k3bsetup root)

kde4_install_auth_actions(org.kde.kcontrol.k3bsetup k3bsetup.actions)
