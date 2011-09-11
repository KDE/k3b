
kde4_add_executable(k3bhelper k3bhelper.cpp k3bhelperprogramitem.cpp)

target_link_libraries(k3bhelper ${KDE4_KDECORE_LIBRARY})

install(TARGETS k3bhelper DESTINATION ${LIBEXEC_INSTALL_DIR})

kde4_install_auth_helper_files(k3bhelper org.kde.k3b root)

kde4_install_auth_actions(org.kde.k3b k3b.actions)
