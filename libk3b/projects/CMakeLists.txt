

add_subdirectory( datacd )
add_subdirectory( audiocd )
add_subdirectory( mixedcd )
add_subdirectory( movixcd )
add_subdirectory( videocd )
add_subdirectory( videodvd )


########### next target ###############


########### install files ###############

install( FILES
  k3bdoc.h
  k3bgrowisofswriter.h
  k3bcdrdaowriter.h
  k3bcdrecordwriter.h
  k3bcdrskinwriter.h
  k3binffilewriter.h
  k3btocfilewriter.h
  k3bcuefileparser.h
  k3bimagefilereader.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR}
  COMPONENT Devel )

