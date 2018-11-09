
set(SOURCES
    bios.cpp
    cpu.cpp
    io.cpp
    machine.cpp
    memory.cpp
  )

add_library(gbc STATIC ${SOURCES})
target_include_directories(gbc PRIVATE ${CMAKE_SOURCE_DIR})
