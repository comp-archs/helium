cmake_minimum_required(VERSION 3.20)
project(helium_assembler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library target (core code)
add_library(helium_asm_lib STATIC
    src/isa.cpp
    src/encoding.cpp
    src/parser.cpp
    src/symbols.cpp
    src/assembler.cpp
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

include(CTest)

if (BUILD_TESTING)
  foreach(test_name IN ITEMS encoding parser symbols assembler)
    add_executable(test_${test_name} tests/test_${test_name}.cpp)
    target_link_libraries(test_${test_name} PRIVATE helium_asm_lib)
    add_test(NAME test_${test_name} COMMAND test_${test_name})
  endforeach()
  add_test(NAME test_cli COMMAND ${CMAKE_COMMAND}
    -DASM=$<TARGET_FILE:helium-asm> -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli-test
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.cmake)
endif()

if(MINGW)
  target_link_options(helium-asm PRIVATE -mconsole)
endif()

option(HELIUM_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

if (HELIUM_WARNINGS_AS_ERRORS)
  set(werror_flag -Werror)
  if (MSVC)
    set(werror_flag /WX)
  endif()
  set(compiled_targets helium_asm_lib helium-asm)
  if (BUILD_TESTING)
    list(APPEND compiled_targets test_encoding test_parser test_symbols test_assembler)
  endif()
  foreach(target IN LISTS compiled_targets)
    target_compile_options(${target} PRIVATE ${werror_flag})
  endforeach()
endif()
