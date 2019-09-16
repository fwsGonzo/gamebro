cmake_minimum_required(VERSION 3.0)
project (gamebro)
include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
conan_basic_setup()
include(os)

os_add_executable(emulador "Gamebro Emulator"
		service.cpp
	)

os_include_directories(emulador PRIVATE ${CMAKE_SOURCE_DIR}/../libgbc)
add_subdirectory(${CMAKE_SOURCE_DIR}/../libgbc libgbc)
os_link_libraries(emulador gbc)

os_diskbuilder(emulador roms)
