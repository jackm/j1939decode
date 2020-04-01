cmake_minimum_required(VERSION 3.1.0)

# Parse the current version from the version header
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/j1939.h" version_defines
        REGEX "#define J1939_DECODE_VERSION_(MAJOR|MINOR|PATCH)")
foreach(ver ${version_defines})
    if(ver MATCHES "#define J1939_DECODE_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
message(STATUS "j1939decode version: ${VERSION}")

project(j1939decode
        VERSION "${VERSION}"
        LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

set(STATIC_LIB static)
set(SHARED_LIB shared)

# uninstall target
if(NOT TARGET uninstall)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)
    add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

add_subdirectory(src)