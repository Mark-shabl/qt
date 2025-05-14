# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\cachedtable_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\cachedtable_autogen.dir\\ParseCache.txt"
  "cachedtable_autogen"
  )
endif()
