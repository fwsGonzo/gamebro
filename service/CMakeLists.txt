cmake_minimum_required(VERSION 3.0)
# IncludeOS install location
if (NOT DEFINED INCLUDEOS_PREFIX)
  if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
    set(INCLUDEOS_PREFIX /usr/local/includeos)
  else()
    set(INCLUDEOS_PREFIX $ENV{INCLUDEOS_PREFIX})
  endif()
endif()

if (NOT EXISTS "${INCLUDEOS_PREFIX}/cmake/os.cmake")
  MESSAGE(FATAL_ERROR "IncludeOS does not appear to be installed at ${INCLUDEOS_PREFIX}")
endif()
list(APPEND CMAKE_MODULE_PATH ${INCLUDEOS_PREFIX}/cmake)

project (gamebro)
include(os)

set(SOURCES
    service.cpp # ...add more here
)

os_add_executable(emulador "Gamebro Emulator" ${SOURCES})

os_include_directories(emulador PRIVATE ${CMAKE_SOURCE_DIR}/libgbc)
add_subdirectory(${CMAKE_SOURCE_DIR}/libgbc)
os_link_libraries(emulador gbc)

os_diskbuilder(emulador roms)
