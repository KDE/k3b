
add_library(k3bdevice SHARED
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
    k3bcdtext.cpp
)

target_include_directories(k3bdevice PUBLIC .)

target_link_libraries(k3bdevice
    PUBLIC
        Qt5::Core
    PRIVATE
        KF5::ConfigCore
        KF5::I18n
        KF5::KIOCore
        KF5::Solid
)

if(CMAKE_SYSTEM_NAME MATCHES "k?FreeBSD")
    target_link_libraries(k3bdevice PRIVATE cam)
endif()

set_target_properties(k3bdevice PROPERTIES
    VERSION ${K3B_LIB_VERSION_MAJOR}.${K3B_LIB_VERSION_MINOR}.${K3B_LIB_VERSION_RELEASE}
    SOVERSION ${K3B_LIB_VERSION_MAJOR}
)

generate_export_header(k3bdevice
    EXPORT_MACRO_NAME LIBK3BDEVICE_EXPORT
    DEPRECATED_MACRO_NAME LIBK3BDEVICE_EXPORT_DEPRECATED
    EXPORT_FILE_NAME k3bdevice_export.h
)

install(TARGETS k3bdevice ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/k3bdevice_export.h DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)

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
    DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel
)
