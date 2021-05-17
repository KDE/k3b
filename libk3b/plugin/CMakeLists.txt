
install( FILES
  k3bplugin.h
  k3bpluginmanager.h
  k3baudiodecoder.h
  k3baudioencoder.h
  k3bpluginconfigwidget.h
  k3bprojectplugin.h
  DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel )

# TODO KF6 remove
install( FILES k3bplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR}) 
