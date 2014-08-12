add_library(k3boggvorbisdecoder MODULE k3boggvorbisdecoder.cpp)

target_link_libraries(k3boggvorbisdecoder k3bdevice k3blib KF5::KDELibs4Support KF5::I18n ${OGGVORBIS_LIBRARIES})

install(TARGETS k3boggvorbisdecoder DESTINATION ${PLUGIN_INSTALL_DIR})

install( FILES  k3boggvorbisdecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
