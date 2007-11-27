
########### next target ###############

set(k3bdevice_LIB_SRCS 
   k3bdevice.cpp 
   k3bdevice_mmc.cpp 
   k3bscsicommand.cpp 
   k3btrack.cpp 
   k3btoc.cpp 
   k3bdevicemanager.cpp 
   k3bmsf.cpp 
   k3bdiskinfo.cpp 
   k3bdeviceglobals.cpp 
   k3bcrc.cpp 
   k3bcdtext.cpp )


kde4_add_library(k3bdevice SHARED ${k3bdevice_LIB_SRCS})

target_link_libraries(k3bdevice
  ${KDE4_KIO_LIBS}
  ${QT_QT3SUPPORT_LIBRARY})

set_target_properties(k3bdevice PROPERTIES VERSION 6.0.0 SOVERSION 6 )
install(TARGETS k3bdevice DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############

install( FILES
  k3bdevicemanager.h
  k3bdevice.h
  k3btoc.h
  k3btrack.h
  k3bdeviceglobals.h
  k3bdiskinfo.h
  k3bcdtext.h
  k3bmsf.h
  k3bdevicetypes.h
  k3bdevice_export.h
  DESTINATION ${INCLUDE_INSTALL_DIR} )


