include_directories(${TAGLIB_INCLUDES})

add_library(k3baudiometainforenamerplugin MODULE k3baudiometainforenamerplugin.cpp)

target_link_libraries(k3baudiometainforenamerplugin
    KF5::I18n
    k3blib
    ${TAGLIB_LIBRARIES}
)

install(TARGETS k3baudiometainforenamerplugin DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILESk3baudiometainforenamerplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
