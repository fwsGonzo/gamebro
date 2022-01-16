
option(GAMEBRO_INDEXED_FRAME "Use indexed pixels for LCD frame" OFF)

set(SOURCES
    apu.cpp
    cpu.cpp
    debug.cpp
    gpu.cpp
    io.cpp
    machine.cpp
    mbc.cpp
    memory.cpp
  )

add_library(gbc STATIC ${SOURCES})

if (GAMEBRO_INDEXED_FRAME)
	target_compile_definitions(gbc PUBLIC GAMEBRO_INDEXED_FRAME=1)
endif()
