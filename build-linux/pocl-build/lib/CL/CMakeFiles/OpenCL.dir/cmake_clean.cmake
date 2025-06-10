file(REMOVE_RECURSE
  ".2"
  "libOpenCL.pdb"
  "libOpenCL.so"
  "libOpenCL.so.2"
  "libOpenCL.so.2.14.0"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/OpenCL.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
