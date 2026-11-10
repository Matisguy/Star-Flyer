# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("starflyer")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GameSources.cmake)

# Compile the Vulkan shaders with the NDK's own compiler. The output is a comma-separated list of
# SPIR-V words that Shader.cpp includes into an array.
file(GLOB STARFLYER_GLSLC_CANDIDATES "${ANDROID_NDK}/shader-tools/*/glslc*")
if (NOT STARFLYER_GLSLC_CANDIDATES)
    message(FATAL_ERROR "glslc not found under ${ANDROID_NDK}/shader-tools")
endif ()
list(GET STARFLYER_GLSLC_CANDIDATES 0 STARFLYER_GLSLC)

set(STARFLYER_SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${STARFLYER_SHADER_DIR})
set(STARFLYER_SHADER_OUTPUTS)
foreach (shader scene.vert scene.frag)
    set(output ${STARFLYER_SHADER_DIR}/${shader}.spv.inc)
    add_custom_command(
            OUTPUT ${output}
            COMMAND ${STARFLYER_GLSLC} -mfmt=num --target-env=vulkan1.0
                    -o ${output} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}
            VERBATIM)
    list(APPEND STARFLYER_SHADER_OUTPUTS ${output})
endforeach ()

# Creates your game shared library. The name must match getLibraries() in MainActivity.java.
add_library(starflyer SHARED
        ${STARFLYER_GAME_SOURCES}
        AndroidBridge.cpp
        ${STARFLYER_SHADER_OUTPUTS})

target_include_directories(starflyer PRIVATE ${STARFLYER_SHADER_DIR})

# Provided by the SDL3 Android archive through prefab
find_package(SDL3 REQUIRED CONFIG)

target_link_libraries(starflyer SDL3::SDL3-shared)
