macro_optional_find_package(Lame)

add_subdirectory( sox )
add_subdirectory( external )

if(OGGVORBIS_FOUND)
   add_subdirectory(ogg)
endif(OGGVORBIS_FOUND)

if(LAME_FOUND)
   add_subdirectory(lame)
endif(LAME_FOUND)
