set(generated "${CMAKE_CURRENT_BINARY_DIR}/x265.rc"
              "${CMAKE_CURRENT_BINARY_DIR}/x265.pc"
              "${CMAKE_CURRENT_BINARY_DIR}/x265.def"
              "${CMAKE_CURRENT_BINARY_DIR}/x265_config.h")

foreach(file ${generated})
  if(EXISTS ${file})
     file(REMOVE ${file})
  endif()
endforeach(file)
