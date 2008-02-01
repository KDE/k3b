macro_optional_find_package(FFmpeg)
macro_optional_find_package(Flac)
macro_optional_find_package(Flac++)
macro_optional_find_package(Mad)
macro_optional_find_package(Muse)
macro_optional_find_package(Sndfile)

add_subdirectory( wave )

if(OGGVORBIS_FOUND)
  add_subdirectory(ogg)
endif(OGGVORBIS_FOUND)

if(FFMPEG_FOUND)
  add_subdirectory(ffmpeg)
endif(FFMPEG_FOUND)

if(FLAC_FOUND AND FLAC++_FOUND)
  add_subdirectory(flac)
endif(FLAC_FOUND AND FLAC++_FOUND)

if(MAD_FOUND)
  add_subdirectory(mp3)
endif(MAD_FOUND)

if(MUSE_FOUND)
  add_subdirectory(musepack)
endif(MUSE_FOUND)

if(SNDFILE_FOUND)
  add_subdirectory(libsndfile)
endif(SNDFILE_FOUND)
