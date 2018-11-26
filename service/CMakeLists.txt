cmake_minimum_required(VERSION 2.8.9)
# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()
include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project (service)

set(SERVICE_NAME "Gamebro Emulator")
set(BINARY       "emulador")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES
    service.cpp
  )

#set(DRIVERS boot_logger ac97 ihda)
#set(STDOUT vga_output)

# include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)

target_include_directories(service PRIVATE ${CMAKE_SOURCE_DIR}/libgbc)
add_subdirectory(${CMAKE_SOURCE_DIR}/libgbc)
target_link_libraries(service gbc)

diskbuilder(roms)
