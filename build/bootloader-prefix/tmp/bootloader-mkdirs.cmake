# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/frameworks/esp-idf-v4.4.2/components/bootloader/subproject"
  "C:/Users/Estrella/esp32/Proyecto_SBC22-T-05/build/bootloader"
  "C:/Users/Estrella/esp32/Proyecto_SBC22-T-05/build/bootloader-prefix"
  "C:/Users/Estrella/esp32/Proyecto_SBC22-T-05/build/bootloader-prefix/tmp"
  "C:/Users/Estrella/esp32/Proyecto_SBC22-T-05/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/Estrella/esp32/Proyecto_SBC22-T-05/build/bootloader-prefix/src"
  "C:/Users/Estrella/esp32/Proyecto_SBC22-T-05/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Estrella/esp32/Proyecto_SBC22-T-05/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
