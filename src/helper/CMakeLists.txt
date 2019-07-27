
add_executable(k3bhelper k3bhelper.cpp k3bhelperprogramitem.cpp)

target_link_libraries(k3bhelper KF5::AuthCore)

install(TARGETS k3bhelper DESTINATION ${LIBEXEC_INSTALL_DIR})

kauth_install_helper_files(k3bhelper org.kde.k3b root)

kauth_install_actions(org.kde.k3b k3b.actions)
