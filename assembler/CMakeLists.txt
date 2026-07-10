cmake_minimum_required(VERSION 3.20)
project(helium_assembler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library target (core code)
add_library(helium_asm_lib STATIC
    src/isa.cpp
    src/encoding.cpp
    # add others as they exist
)

target_include_directories(helium_asm_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Executable target (CLI)
add_executable(helium-asm
    src/main.cpp
)

target_link_libraries(helium-asm
    PRIVATE helium_asm_lib
)

if(MINGW)
  target_link_options(helium-asm PRIVATE -mconsole)
endif()

option(HELIUM_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

if (HELIUM_WARNINGS_AS_ERRORS)
  if (MSVC)
    target_compile_options(helium_asm_lib PRIVATE /WX)
    target_compile_options(helium-asm PRIVATE /WX)
  else()
    target_compile_options(helium_asm_lib PRIVATE -Werror)
    target_compile_options(helium-asm PRIVATE -Werror)
  endif()
endif()
