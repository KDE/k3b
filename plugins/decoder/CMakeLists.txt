macro_optional_find_package(OggVorbis)
macro_optional_find_package(FFmpeg)
macro_optional_find_package(Flac)
macro_optional_find_package(Mad)
if(OGGVORBIS_FOUND)
   add_subdirectory(ogg)
endif(OGGVORBIS_FOUND)
if(FFMPEG_FOUND)
   add_subdirectory(ffmpeg)
endif(FFMPEG_FOUND)
if(FLAC_FOUND)
   add_subdirectory(flac)
endif(FLAC_FOUND)
if(MAD_FOUND)
   add_subdirectory(mp3)
endif(MAD_FOUND)

add_subdirectory( wave )

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(DECAIFFDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(MPCDIR)")

########### install files ###############




#original Makefile.am contents follow:

#if include_MP3
#DECMP3DIR = mp3
#endif
#
#DECAIFFDIR = libsndfile
#endif
#
#if include_MPC
#MPCDIR = musepack
#endif
#
#SUBDIRS = wave $(DECOGGDIR) $(DECMP3DIR) $(DECFLACDIR) $(DECAIFFDIR) $(FFMPEGDIR) $(MPCDIR)
