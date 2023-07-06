cmake_minimum_required(VERSION 3.22)

project(triangle)

# Files
file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsli)

file(GLOB_RECURSE HEADER_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

file(GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)


add_executable(triangle ${SOURCE_FILES})

target_sources(triangle PRIVATE ${SHADER_FILES})
source_group("Shaders" FILES ${SHADER_FILES})
set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

target_sources(triangle PRIVATE ${HEADER_FILES} ${SHADER_FILES})
target_include_directories(triangle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# link limbo
target_link_libraries(triangle PUBLIC limbo glfw)
target_include_directories(triangle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../src/)

target_compile_features(triangle PUBLIC cxx_std_20)
if (WIN32)
target_compile_options(triangle PRIVATE
    /W3 /WX
    -D_HAS_EXCEPTIONS=0
    -DLIMBO_WINDOWS=1
    -DLIMBO_LINUX=0
)
else()
target_compile_options(triangle PRIVATE
    -Wall
    -Werror
    -Wconversion
    -Wno-error=comment
    -fno-exceptions
    -DLIMBO_WINDOWS=0
    -DLIMBO_LINUX=1
)
endif()


# copy DLL's
add_custom_command(
    TARGET triangle
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/d3d12/bin/D3D12Core.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/D3D12/D3D12Core.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/d3d12/bin/D3D12Core.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/D3D12/D3D12Core.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/d3d12/bin/D3D12Core.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/D3D12/D3D12Core.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/d3d12/bin/D3D12Core.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/D3D12/D3D12Core.dll)

add_custom_command(
    TARGET triangle
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/d3d12/bin/d3d12SDKLayers.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/D3D12/d3d12SDKLayers.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/d3d12/bin/d3d12SDKLayers.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/D3D12/d3d12SDKLayers.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/d3d12/bin/d3d12SDKLayers.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/D3D12/d3d12SDKLayers.dll)

set_target_properties(triangle PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(triangle PROPERTIES FOLDER "samples")