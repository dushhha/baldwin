cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(baldwin)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE_DIR ${ROOT_DIR}/src)
set(THIRD_PARTY_DIR ${ROOT_DIR}/third_party)
set(SHADER_DIR ${ROOT_DIR}/shaders)
set(COMPILED_SHADER_DIR ${CMAKE_BINARY_DIR}/shaders)
set(ASSETS_DIR ${ROOT_DIR}/assets)

find_package(Vulkan REQUIRED)
if(NOT DEFINED Vulkan_GLSLC_EXECUTABLE)
  message(
    FATAL_ERROR
      "glslc not found! Ensure Vulkan SDK is installed and Vulkan_GLSLC_EXECUTABLE is set."
  )
endif()
# Shader compilation pipeline
file(GLOB SHADERS "${SHADER_DIR}/*.frag.glsl" "${SHADER_DIR}/*.vert.glsl"
     "${SHADER_DIR}/*.comp.glsl")
file(MAKE_DIRECTORY ${COMPILED_SHADER_DIR})
foreach(SHADER IN LISTS SHADERS)
  get_filename_component(FILENAME ${SHADER} NAME)
  get_filename_component(SHADER_NAME ${SHADER} NAME_WLE)
  add_custom_command(
    OUTPUT ${COMPILED_SHADER_DIR}/${SHADER_NAME}.spv
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${SHADER} -o
            ${COMPILED_SHADER_DIR}/${SHADER_NAME}.spv
    DEPENDS ${SHADER}
    COMMENT "Compiling ${FILENAME}")
  list(APPEND COMPILED_SHADERS ${COMPILED_SHADER_DIR}/${SHADER_NAME}.spv)
endforeach()
add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})

# Final target
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.c")
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_dependencies(${PROJECT_NAME} compile_shaders)

add_subdirectory(${THIRD_PARTY_DIR}/glfw-3.4)
add_subdirectory(${THIRD_PARTY_DIR}/vk-bootstrap)
add_subdirectory(${THIRD_PARTY_DIR}/vma)
# add_subdirectory(${THIRD_PARTY_DIR}/imgui)
add_subdirectory(${THIRD_PARTY_DIR}/fastgltf)
find_package(glm CONFIG REQUIRED)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${ROOT_DIR}/include ${SOURCE_DIR} ${THIRD_PARTY_DIR}/glfw-3.4/include
         ${Vulkan_INCLUDE_DIRS} ${THIRD_PARTY_DIR}/vma
         ${THIRD_PARTY}/fastgltf/include)
target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE glfw
          ${Vulkan_LIBRARIES}
          vk-bootstrap::vk-bootstrap
          GPUOpen::VulkanMemoryAllocator
          # imgui
          glm::glm
          fastgltf)

# Copy assets
file(COPY ${ASSETS_DIR} DESTINATION ${CMAKE_BINARY_DIR})
