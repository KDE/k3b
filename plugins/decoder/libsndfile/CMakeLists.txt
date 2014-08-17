add_library(k3blibsndfiledecoder MODULE k3blibsndfiledecoder.cpp)

target_include_directories(k3blibsndfiledecoder PRIVATE ${SNDFILE_INCLUDE_DIR})

target_link_libraries(k3blibsndfiledecoder k3bdevice k3blib KF5::I18n ${SNDFILE_LIBRARIES})

install(TARGETS k3blibsndfiledecoder DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES  k3blibsndfiledecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR})
