
check_cxx_source_compiles("
#include <FLAC++/metadata.h>
int main()
{
    FLAC::Metadata::VorbisComment* vc;
    vc->get_vendor_string().get_field();
    return 0;
}
" FLAC_LESS_THAN_1_1_1)

if(NOT FLAC_LESS_THAN_1_1_1)
    set(FLAC_NEWER_THAN_1_1_1 1)
endif()

configure_file(config-flac.h.cmake config-flac.h)

kcoreaddons_add_plugin(k3bflacdecoder
    SOURCES k3bflacdecoder.cpp
    INSTALL_NAMESPACE "k3b_plugins")

target_include_directories(k3bflacdecoder PRIVATE ${FLAC++_INCLUDE_DIR} ${FLAC_INCLUDE_DIR})

target_link_libraries(k3bflacdecoder k3bdevice k3blib KF6::I18n ${FLAC++_LIBRARIES} ${FLAC_LIBRARIES})

if(ENABLE_TAGLIB)
    target_link_libraries(k3bflacdecoder Taglib::Taglib)
endif()
