
check_cxx_source_compiles("
#include <FLAC++/metadata.h>
int main()
{
  FLAC::Metadata::VorbisComment* vc;
  vc->get_vendor_string().get_field();
  return 0;
}
" FLAC_LESS_THAN_1_1_1 )

if( NOT FLAC_LESS_THAN_1_1_1)
  set(FLAC_NEWER_THAN_1_1_1 1)
endif(NOT FLAC_LESS_THAN_1_1_1)

configure_file(config-flac.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-flac.h )

########### next target ###############

set(k3bflacdecoder_PART_SRCS k3bflacdecoder.cpp )

kde4_add_plugin(k3bflacdecoder ${k3bflacdecoder_PART_SRCS})

target_link_libraries(k3bflacdecoder
  k3b k3bdevice
  ${KDE4_KDEUI_LIBS}
  ${FLAC++_LIBRARIES}
  ${FLAC_LIBRARIES} )

if(ENABLE_TAGLIB)
  target_link_libraries(k3bflacdecoder
    ${TAGLIB_LIBRARIES})
endif(ENABLE_TAGLIB)

install(TARGETS k3bflacdecoder DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES  k3bflacdecoder.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

