include_directories(
  ../../libk3b/projects/audiocd/
  ../../libk3b/projects/datacd/
  ../../libk3b/projects/
  ../../libk3b/tools/
)

add_subdirectory( audioprojectcddb )

MESSAGE(STATUS "audiometainforenamer disable for the moment need to port to
strigi ?")
#add_subdirectory( audiometainforenamer )

