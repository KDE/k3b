add_library(k3bwavedecoder MODULE k3bwavedecoder.cpp)

target_link_libraries(k3bwavedecoder k3bdevice k3blib KF5::KDELibs4Support KF5::I18n)

install(TARGETS k3bwavedecoder DESTINATION ${PLUGIN_INSTALL_DIR})

install( FILES  k3bwavedecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
