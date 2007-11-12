
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install( FILES  cdi_imag.rtf cdi_text.fnt cdi_vcd.app cdi_vcd.cfg vcd_on_cdi_41.pdf icdia.htm DESTINATION ${DATA_INSTALL_DIR}/k3b/cdi )



#original Makefile.am contents follow:

#
#cdidir = $(kde_datadir)/k3b/cdi
#cdi_DATA = cdi_imag.rtf cdi_text.fnt cdi_vcd.app cdi_vcd.cfg vcd_on_cdi_41.pdf icdia.htm
#
#EXTRA_DIST = $(cdi_DATA)
