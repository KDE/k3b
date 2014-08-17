include_directories(${TAGLIB_INCLUDES})

add_library(k3baudiometainforenamerplugin MODULE k3baudiometainforenamerplugin.cpp)

target_link_libraries(k3baudiometainforenamerplugin
    ${TAGLIB_LIBRARIES}
    k3bdevice
    k3blib
    KF5::KIOCore
    KF5::I18n
    KF5::WidgetsAddons
    Qt5::Widgets
)

install(TARGETS k3baudiometainforenamerplugin DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES k3baudiometainforenamerplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
