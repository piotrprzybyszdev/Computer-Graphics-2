cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CONFIGURATION_TYPES Release Profile Debug Trace)

project(gk LANGUAGES CXX C)

include(${CMAKE_SOURCE_DIR}/cmake/Utils.cmake)

add_configuration(TRACE DEBUG TRUE)
add_configuration(PROFILE RELEASE FALSE)

option(REF_BUILD_DEMOS OFF)

add_subdirectory(robot)
add_subdirectory(duck)
add_subdirectory(Rendering-Engine-Framework)
add_subdirectory(vendor/glm)

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "duck")
endif()