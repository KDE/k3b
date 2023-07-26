
install( FILES
  k3bplugin.h
  k3bpluginmanager.h
  k3baudiodecoder.h
  k3baudioencoder.h
  k3bpluginconfigwidget.h
  k3bprojectplugin.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR} COMPONENT Devel )

if (QT_MAJOR_VERSION STREQUAL "5")
    install( FILES k3bplugin.desktop DESTINATION ${KDE_INSTALL_DATADIR}/kio)
endif()
