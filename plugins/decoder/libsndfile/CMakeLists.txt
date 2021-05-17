kcoreaddons_add_plugin(k3blibsndfiledecoder SOURCES k3blibsndfiledecoder.cpp INSTALL_NAMESPACE "k3b_plugins")
kcoreaddons_desktop_to_json(k3blibsndfiledecoder "k3blibsndfiledecoder.desktop" SERVICE_TYPES ${CMAKE_SOURCE_DIR}/libk3b/plugin/k3bplugin.desktop)

target_include_directories(k3blibsndfiledecoder PRIVATE ${SNDFILE_INCLUDE_DIR})

target_link_libraries(k3blibsndfiledecoder k3bdevice k3blib KF5::I18n ${SNDFILE_LIBRARIES})
