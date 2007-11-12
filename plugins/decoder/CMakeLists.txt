
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

add_subdirectory( wave )

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(DECOGGDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(DECMP3DIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(DECFLACDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(DECAIFFDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(FFMPEGDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(MPCDIR)")

########### install files ###############




#original Makefile.am contents follow:

#
#if include_OGG
#DECOGGDIR = ogg
#endif
#
#if include_MP3
#DECMP3DIR = mp3
#endif
#
#if include_FLAC
#DECFLACDIR = flac
#endif
#
#if include_AIFF
#DECAIFFDIR = libsndfile
#endif
#
#if include_FFMPEG
#FFMPEGDIR = ffmpeg
#endif
#
#if include_MPC
#MPCDIR = musepack
#endif
#
#SUBDIRS = wave $(DECOGGDIR) $(DECMP3DIR) $(DECFLACDIR) $(DECAIFFDIR) $(FFMPEGDIR) $(MPCDIR)
