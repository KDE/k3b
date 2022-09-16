
install( FILES
  k3bplugin.h
  k3bpluginmanager.h
  k3baudiodecoder.h
  k3baudioencoder.h
  k3bpluginconfigwidget.h
  k3bprojectplugin.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR} COMPONENT Devel )

# TODO KF6 remove
install( FILES k3bplugin.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR}) 
