project(k3bdevice)

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


if(ENABLE_HAL_SUPPORT)
  set(k3bdevice_LIB_SRCS 
    ${k3bdevice_LIB_SRCS}
    k3bhalconnection.cpp)
endif(ENABLE_HAL_SUPPORT)

kde4_add_library(k3bdevice SHARED ${k3bdevice_LIB_SRCS})

target_link_libraries(k3bdevice
  ${KDE4_KDECORE_LIBS}
  ${KDE4_SOLID_LIBS}
)

set_target_properties(k3bdevice PROPERTIES VERSION ${K3B_LIB_VERSION_MAJOR}.${K3B_LIB_VERSION_MINOR}.${K3B_LIB_VERSION_RELEASE} SOVERSION ${K3B_LIB_VERSION_MAJOR})
install(TARGETS k3bdevice ${INSTALL_TARGETS_DEFAULT_ARGS} )


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
  DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel )


