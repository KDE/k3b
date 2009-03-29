install( FILES
  k3b_audiocd_rip.desktop
  k3b_copy_disc.desktop
  k3b_create_data_project_from_blank_medium.desktop
  k3b_create_audio_cd_from_blank_medium.desktop
  DESTINATION
  ${DATA_INSTALL_DIR}/solid/actions )

if(ENABLE_DVD_RIPPING)
  install( FILES
    k3b_videodvd_rip.desktop
    DESTINATION
    ${DATA_INSTALL_DIR}/solid/actions )
endif(ENABLE_DVD_RIPPING)

install( FILES
  k3b_create_audio_cd.desktop
  k3b_create_data_project.desktop
  k3b_create_video_cd.desktop
  k3b_write_bin_image.desktop
  k3b_write_iso_image.desktop
  DESTINATION
  ${SERVICES_INSTALL_DIR}/ServiceMenus )
