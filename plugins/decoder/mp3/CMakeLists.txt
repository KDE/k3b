add_library(k3bmaddecoder MODULE k3bmad.cpp)

target_link_libraries(k3bmaddecoder k3blib k3bdevice KF5::KDELibs4Support KF5::I18n ${MAD_LIBRARIES})

if(ENABLE_TAGLIB)
    target_link_libraries(k3bmaddecoder ${TAGLIB_LIBRARIES})
endif()

install(TARGETS k3bmaddecoder DESTINATION ${PLUGIN_INSTALL_DIR} )

install(FILES  k3bmaddecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
