install( FILES
  k3b_audiocd_rip.desktop
  k3b_copy_disc.desktop
  k3b_create_data_project_from_blank_medium.desktop
  k3b_create_audio_cd_from_blank_medium.desktop
  DESTINATION
  ${KDE_INSTALL_DATADIR}/solid/actions )

if(ENABLE_DVD_RIPPING)
  install( FILES
    k3b_videodvd_rip.desktop
    DESTINATION
    ${KDE_INSTALL_DATADIR}/solid/actions )
endif()

install(PROGRAMS # They need to be executable to be considered authorized since this is a non-standard location
  k3b_create_audio_cd.desktop
  k3b_create_data_project.desktop
  k3b_create_video_cd.desktop
  k3b_write_bin_image.desktop
  k3b_write_iso_image.desktop
  DESTINATION ${KDE_INSTALL_DATADIR}/kio/servicemenus
  )
