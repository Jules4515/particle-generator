cmake_minimum_required(VERSION 3.20)
project(ParticlesVulkan CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan
find_package(Vulkan REQUIRED)

include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# VMA
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(vma)

# ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.8-docking
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# JSON
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

file(GLOB SOURCES "src/*.cpp")
add_executable(ParticlesVulkan
    ${SOURCES}
    src/VulkanEngine.hpp
)

target_include_directories(ParticlesVulkan PRIVATE
    ${vma_SOURCE_DIR}/include
)

target_link_libraries(ParticlesVulkan PRIVATE
    Vulkan::Vulkan
    glfw
    glm
    imgui
    nlohmann_json::nlohmann_json
)

# Shader compilation
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADERS 
    ${SHADER_DIR}/particle.vert
    ${SHADER_DIR}/particle.frag
    ${SHADER_DIR}/particle.comp
)
foreach(SHADER ${SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    add_custom_command(
        TARGET ParticlesVulkan PRE_BUILD
        COMMAND glslc ${SHADER} -o ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.spv
        DEPENDS ${SHADER}
        COMMENT "Compiling ${FILE_NAME}"
    )
endforeach()
