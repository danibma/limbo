cmake_minimum_required(VERSION 3.22)

project(limbo)

# Files
file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsli)

# enable folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# set the cmake projects into CMakePredefinedTargets
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER)

option(LIMBO_BUILD_SAMPLES "Build limbo samples" ON)

file(GLOB_RECURSE HEADER_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

file(GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(limbo ${SOURCE_FILES})

target_sources(limbo PRIVATE ${SHADER_FILES})
source_group("Shaders" FILES ${SHADER_FILES})
set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

target_sources(limbo PRIVATE ${HEADER_FILES} ${SHADER_FILES})
target_include_directories(limbo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add samples
if (LIMBO_BUILD_SAMPLES)
    message("Building limbo samples")

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/samples/triangle)

    # Set up startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT triangle)
endif()

target_compile_features(limbo PUBLIC cxx_std_20)

if (WIN32)
target_compile_options(limbo PRIVATE
    /W3 /WX
    -D_HAS_EXCEPTIONS=0
    -DLIMBO_WINDOWS=1
    -DLIMBO_LINUX=0
)
else()
target_compile_options(limbo PRIVATE
    -Wall
    -Werror
    -Wconversion
    -Wno-error=comment
    -fno-exceptions
    -DLIMBO_WINDOWS=0
    -DLIMBO_LINUX=1
)
endif()
