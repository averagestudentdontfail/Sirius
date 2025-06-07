# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/send2/.projects/Sirius/build/_deps/glfw-src"
  "/home/send2/.projects/Sirius/build/_deps/glfw-build"
  "/home/send2/.projects/Sirius/build/_deps/glfw-subbuild/glfw-populate-prefix"
  "/home/send2/.projects/Sirius/build/_deps/glfw-subbuild/glfw-populate-prefix/tmp"
  "/home/send2/.projects/Sirius/build/_deps/glfw-subbuild/glfw-populate-prefix/src/glfw-populate-stamp"
  "/home/send2/.projects/Sirius/build/_deps/glfw-subbuild/glfw-populate-prefix/src"
  "/home/send2/.projects/Sirius/build/_deps/glfw-subbuild/glfw-populate-prefix/src/glfw-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/send2/.projects/Sirius/build/_deps/glfw-subbuild/glfw-populate-prefix/src/glfw-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/send2/.projects/Sirius/build/_deps/glfw-subbuild/glfw-populate-prefix/src/glfw-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
