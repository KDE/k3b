if(BUILD_SOX_ENCODER_PLUGIN)
    add_subdirectory( sox )
endif()

if(BUILD_EXTERNAL_ENCODER_PLUGIN)
    add_subdirectory( external )
endif()

if(BUILD_OGGVORBIS_ENCODER_PLUGIN)
    add_subdirectory(ogg)
endif()

if(BUILD_LAME_ENCODER_PLUGIN)
    add_subdirectory(lame)
endif()
