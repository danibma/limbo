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
option(LIMBO_BUILD_TESTS "Build limbo tests" ON)

file(GLOB_RECURSE HEADER_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/device.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resourcemanager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/limbo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/buffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/texture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shadercompiler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resourcepool.h)

file(GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/limbo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shadercompiler.cpp)

# Vulkan specific
file(GLOB_RECURSE VULKAN_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan/*.cpp)

# D3D12 specific
file(GLOB_RECURSE D3D12_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/D3D12/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/D3D12/*.cpp)

add_library(limbo ${SOURCE_FILES} ${VULKAN_FILES} ${D3D12_FILES})

target_sources(limbo PRIVATE ${SHADER_FILES})
source_group("Shaders" FILES ${SHADER_FILES})
set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

target_sources(limbo PRIVATE ${HEADER_FILES} ${SHADER_FILES})
source_group("Source" FILES ${SOURCE_FILES} ${HEADER_FILES})
source_group("Source/Vulkan" FILES ${VULKAN_FILES})
source_group("Source/D3D12" FILES ${D3D12_FILES})
target_include_directories(limbo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Defines
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNO_LOG")

# Vulkan headers
target_include_directories(limbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/VulkanHeaders)

# dxc
target_include_directories(limbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/inc)
target_link_libraries(limbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/lib/x64/dxcompiler.lib)

# d3d12
target_include_directories(limbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3d12/include)

# Add samples
if (LIMBO_BUILD_SAMPLES)
    message("Building limbo samples")

    # Samples
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/samples/triangle)

    # GLFW
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/samples/third_party/glfw)
    set_target_properties(glfw PROPERTIES FOLDER "samples/third_party/glfw3")
    set_target_properties(update_mappings PROPERTIES FOLDER "samples/third_party/glfw3")
    set_target_properties(uninstall PROPERTIES FOLDER "samples/third_party/glfw3")

    # Set up startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT triangle)
endif()

# Tests
if (LIMBO_BUILD_TESTS)
    message("Building limbo tests")

    file(GLOB_RECURSE TESTS_FILES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

    add_executable(limbo_tests ${TESTS_FILES})

    target_include_directories(limbo_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/third_party)

    target_link_libraries(limbo_tests PUBLIC limbo)
    target_include_directories(limbo_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/)

    set_target_properties(limbo_tests PROPERTIES 
        VS_DEBUGGER_COMMAND_ARGUMENTS "-s -b")

    if (WIN32)
    target_compile_options(limbo_tests PRIVATE
        /W3 /WX
        -D_HAS_EXCEPTIONS=0
        -DLIMBO_WINDOWS=1
        -DLIMBO_LINUX=0
    )
    else()
    target_compile_options(limbo_tests PRIVATE
        -Wall
        -Werror
        -Wconversion
        -Wno-error=comment
        -fno-exceptions
        -DLIMBO_WINDOWS=0
        -DLIMBO_LINUX=1
    )
    endif()
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
