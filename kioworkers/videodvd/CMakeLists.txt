
add_library(kio_videodvd MODULE videodvd.cpp)
set_target_properties(kio_videodvd PROPERTIES OUTPUT_NAME "videodvd")

target_link_libraries(kio_videodvd KF5::KIOCore KF5::I18n k3blib k3bdevice)

ecm_qt_declare_logging_category(kio_videodvd
    HEADER videodvd_log.h
    IDENTIFIER KIO_VIDEODVD_LOG
    CATEGORY_NAME kf.kio.workers.videodvd
    DESCRIPTION "videodvd KIO worker"
    EXPORT K3B
)

generate_export_header(kio_videodvd
    EXPORT_MACRO_NAME VIDEODVD_EXPORT
    DEPRECATED_MACRO_NAME VIDEODVD_EXPORT_DEPRECATED
    EXPORT_FILE_NAME videodvd_export.h
)

install(TARGETS kio_videodvd DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

install( FILES  videodvd.desktop DESTINATION ${KDE_INSTALL_DATADIR}/konqsidebartng/virtual_folders/services)
