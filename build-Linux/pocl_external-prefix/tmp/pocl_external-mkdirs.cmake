# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/send2/.projects/Sirius/deps/pocl"
  "/home/send2/.projects/Sirius/build-Linux/pocl-build"
  "/home/send2/.projects/Sirius/build-Linux/pocl-install"
  "/home/send2/.projects/Sirius/build-Linux/pocl_external-prefix/tmp"
  "/home/send2/.projects/Sirius/build-Linux/pocl_external-prefix/src/pocl_external-stamp"
  "/home/send2/.projects/Sirius/build-Linux/pocl_external-prefix/src"
  "/home/send2/.projects/Sirius/build-Linux/pocl_external-prefix/src/pocl_external-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/send2/.projects/Sirius/build-Linux/pocl_external-prefix/src/pocl_external-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/send2/.projects/Sirius/build-Linux/pocl_external-prefix/src/pocl_external-stamp${cfgdir}") # cfgdir has leading slash
endif()
