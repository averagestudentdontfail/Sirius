file(REMOVE_RECURSE
  ".2"
  "libpocl.pdb"
  "libpocl.so"
  "libpocl.so.2"
  "libpocl.so.2.14.0"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/pocl.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
