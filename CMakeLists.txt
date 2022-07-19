
option(GAMEBRO_INDEXED_FRAME "Use indexed pixels for LCD frame" OFF)

set(SOURCES
    libgbc/apu.cpp
    libgbc/cpu.cpp
    libgbc/debug.cpp
    libgbc/gpu.cpp
    libgbc/io.cpp
    libgbc/machine.cpp
    libgbc/mbc.cpp
    libgbc/memory.cpp
  )

add_library(gbc STATIC ${SOURCES})
target_include_directories(gbc PUBLIC .)

if (GAMEBRO_INDEXED_FRAME)
	target_compile_definitions(gbc PUBLIC GAMEBRO_INDEXED_FRAME=1)
endif()
