cmake_minimum_required(VERSION 3.21)
project(strawberry C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

list(APPEND CMAKE_C_FLAGS
    -pedantic-errors
    -Wno-gnu-empty-struct
    -Wno-empty-translation-unit
)
string(JOIN " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

file(GLOB_RECURSE STRC_SOURCES CONFIGURE_DEPENDS src/*.c)
file(GLOB_RECURSE STRC_HEADERS CONFIGURE_DEPENDS src/*.h)

add_executable(strawberry ${STRC_SOURCES} ${STRC_HEADERS})
