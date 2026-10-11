cmake_minimum_required(VERSION 3.10)
project(WakeOnLAN VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(wol_lib STATIC wol.cpp)
target_include_directories(wol_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(wol main.cpp)
target_include_directories(wol PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(wol PRIVATE wol_lib)

if(WIN32)
  target_link_libraries(wol_lib PUBLIC ws2_32)
endif()

if(MSVC)
  target_compile_options(wol_lib PRIVATE /W4)
  target_compile_options(wol PRIVATE /W4)
else()
  target_compile_options(wol_lib PRIVATE -Wall -Wextra)
  target_compile_options(wol PRIVATE -Wall -Wextra)
endif()

enable_testing()
add_executable(test_wol tests/test_wol.cpp)
target_include_directories(test_wol PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_wol PRIVATE wol_lib)
add_test(NAME wol_unit_tests COMMAND test_wol)