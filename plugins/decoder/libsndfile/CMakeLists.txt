kcoreaddons_add_plugin(k3blibsndfiledecoder
    SOURCES k3blibsndfiledecoder.cpp
    INSTALL_NAMESPACE "k3b_plugins")

target_include_directories(k3blibsndfiledecoder PRIVATE ${SNDFILE_INCLUDE_DIR})

target_link_libraries(k3blibsndfiledecoder k3bdevice k3blib KF6::I18n ${SNDFILE_LIBRARIES})
