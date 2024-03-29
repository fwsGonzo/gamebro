cmake_minimum_required (VERSION 3.5.1)
project (gamebro C CXX)

set(CMAKE_CXX_STANDARD 17)
set(COMMON "-g -O2 -march=native -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON}")

option(LIBCPP       "Enable libc++" OFF)
option(PERFORMANCE  "Enable maximum performance" OFF)
option(DEBUGGING    "Enable debugging" OFF)
option(GPROF        "Enable profiling with gprof" OFF)
option(PGO_ENABLE   "Enable guided profiling (PGO)" OFF)
option(PGO_GENERATE "PGO is in profile generating mode" ON)
option(SANITIZE     "Enable undefined- and address sanitizers" OFF)
option(LIBFUZZER    "Enable in-process fuzzer" OFF)
option(ENABLE_LTO   "Enable LTO for use with Clang/GCC" OFF)

if (PERFORMANCE)
  if (DEBUGGING)
    message(FATAL_ERROR "You can not mix PERFORMANCE and DEBUGGING")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
endif()

if (DEBUGGING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
endif()

if (ENABLE_LTO)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -flto")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
  endif()
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
endif()

if(GPROF)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fno-omit-frame-pointer")
endif()

if (PGO_ENABLE)
  if (PGO_GENERATE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-dir=$ENV{HOME}/pgo -fprofile-generate")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-dir=$ENV{HOME}/pgo -fprofile-use")
  endif()
endif()

if(SANITIZE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fsanitize=address")
endif()
if(LIBFUZZER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer-no-link")
  add_definitions(-DLIBFUZZER_ENABLED)
endif()

if (LIBCPP)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

add_subdirectory(.. libgbc)
add_subdirectory(src)

add_executable(gamebro ${SOURCES})
target_link_libraries(gamebro gbc)

target_include_directories(gamebro PRIVATE ../ext)
