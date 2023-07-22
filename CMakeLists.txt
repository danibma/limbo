cmake_minimum_required(VERSION 3.22)

function(GroupSourcesByFolder target)
  set(SOURCE_GROUP_DELIMITER "/")
  set(last_dir "")
  set(files "")

  get_target_property(sources ${target} SOURCES)

  foreach(file ${sources})
    file(RELATIVE_PATH relative_file "${PROJECT_SOURCE_DIR}" ${file})
    get_filename_component(dir "${relative_file}" PATH)
    if(NOT "${dir}" STREQUAL "${last_dir}")
      if(files)
        source_group("${last_dir}" FILES ${files})
      endif()
      set(files "")
    endif()
    set(files ${files} ${file})
    set(last_dir "${dir}")
  endforeach()

  if(files)
    source_group("${last_dir}" FILES ${files})
  endif()
endfunction()

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

option(LIMBO_BUILD_TESTS "Build limbo tests" OFF)

file(GLOB_RECURSE CPP_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

add_executable(limbo ${CPP_FILES})
GroupSourcesByFolder(limbo)

target_sources(limbo PRIVATE ${SHADER_FILES})
source_group("shaders" FILES ${SHADER_FILES})
set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

target_include_directories(limbo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Defines
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNO_LOG")

# GLFW
target_link_libraries(limbo PUBLIC glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw)
set_target_properties(glfw PROPERTIES FOLDER "third_party/glfw3")
set_target_properties(update_mappings PROPERTIES FOLDER "third_party/glfw3")
set_target_properties(uninstall PROPERTIES FOLDER "third_party/glfw3")

# glm
target_include_directories(limbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm)
# dxc
target_include_directories(limbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/inc)
target_link_libraries(limbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/lib/x64/dxcompiler.lib)

# d3d12
target_include_directories(limbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3d12/include)

target_include_directories(limbo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/)

target_compile_features(limbo PUBLIC cxx_std_20)

target_compile_options(limbo PRIVATE
    /W3 /WX /MP
    -D_HAS_EXCEPTIONS=0
)

# copy DLL's
add_custom_command(
    TARGET limbo
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3d12/bin/D3D12Core.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/D3D12/D3D12Core.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3d12/bin/D3D12Core.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/D3D12/D3D12Core.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3d12/bin/D3D12Core.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/D3D12/D3D12Core.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3d12/bin/D3D12Core.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/D3D12/D3D12Core.dll)

add_custom_command(
    TARGET limbo
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/dxil.dll)

add_custom_command(
    TARGET limbo
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3d12/bin/d3d12SDKLayers.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/D3D12/d3d12SDKLayers.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3d12/bin/d3d12SDKLayers.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/D3D12/d3d12SDKLayers.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/d3d12/bin/d3d12SDKLayers.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/D3D12/d3d12SDKLayers.dll)

set_target_properties(limbo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT limbo)