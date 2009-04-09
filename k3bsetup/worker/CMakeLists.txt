########### next target ###############

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${POLKITQT_INCLUDE_DIR}
    ..
)
 
set(K3BSETUPWORKER_INSTALL_DIR ${LIBEXEC_INSTALL_DIR})

qt4_add_dbus_adaptor(
    k3bsetupworker_adaptor_SRCS
    org.k3b.setup.xml
    k3bsetupworker.h
    K3b::Setup::Worker
)

set(k3bsetupworker_SRCS
    main.cpp
    k3bsetupworker.cpp
    ../k3bsetupprogramitem.cpp
)

kde4_add_executable(k3bsetup_worker
    ${k3bsetupworker_SRCS}
    ${k3bsetupworker_adaptor_SRCS}
)

target_link_libraries(k3bsetup_worker
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${POLKITQT_CORE_LIBRARY}
)

dbus_add_activation_system_service(org.k3b.setup.service.in)
install(TARGETS k3bsetup_worker DESTINATION ${K3BSETUPWORKER_INSTALL_DIR} )
install(FILES org.k3b.setup.conf DESTINATION ${SYSCONF_INSTALL_DIR}/dbus-1/system.d)
install(FILES org.k3b.setup.policy DESTINATION ${POLICY_FILES_INSTALL_DIR}) 
install(FILES org.k3b.setup.xml DESTINATION ${KDE4_DBUS_INTERFACES_DIR} )
