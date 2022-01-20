if(BUILD_WAVE_DECODER_PLUGIN)
    add_subdirectory( wave )
endif()

if(BUILD_OGGVORBIS_DECODER_PLUGIN)
    add_subdirectory(ogg)
endif()

if(BUILD_FLAC_DECODER_PLUGIN)
    add_subdirectory(flac)
endif()

if(BUILD_MAD_DECODER_PLUGIN)
    add_subdirectory(mp3)
endif()

if(BUILD_MUSE_DECODER_PLUGIN)
    add_subdirectory(musepack)
endif()

if(BUILD_SNDFILE_DECODER_PLUGIN)
    add_subdirectory(libsndfile)
endif()
