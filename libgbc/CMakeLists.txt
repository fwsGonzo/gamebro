
set(SOURCES
    bios.cpp
    cpu.cpp
    gpu.cpp
    io.cpp
    machine.cpp
    mbc1.cpp
    memory.cpp
  )

add_library(gbc STATIC ${SOURCES})
target_include_directories(gbc PRIVATE ${CMAKE_SOURCE_DIR})
