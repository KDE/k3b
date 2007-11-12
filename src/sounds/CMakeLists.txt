
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install( FILES  k3b_success1.wav k3b_error1.wav k3b_wait_media1.wav DESTINATION ${SOUND_INSTALL_DIR} )



#original Makefile.am contents follow:

#
#soundsdir = $(kde_sounddir)
#sounds_DATA = k3b_success1.wav k3b_error1.wav k3b_wait_media1.wav
#
#EXTRA_DIST = $(sounds_DATA)
