
add_library(kio_videodvd MODULE videodvd.cpp)

target_link_libraries(kio_videodvd KF5::KIOCore KF5::I18n k3blib k3bdevice)

generate_export_header(kio_videodvd
    EXPORT_MACRO_NAME VIDEODVD_EXPORT
    DEPRECATED_MACRO_NAME VIDEODVD_EXPORT_DEPRECATED
    EXPORT_FILE_NAME videodvd_export.h
)

install(TARGETS kio_videodvd DESTINATION ${PLUGIN_INSTALL_DIR})

install( FILES  videodvd.protocol DESTINATION ${SERVICES_INSTALL_DIR})

install( FILES  videodvd.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/services)
