cmake_minimum_required(VERSION 3.10)
project(AdvancedEdit)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "/DEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# Include SDL3 headers and link prebuilt libraries
add_library(SDL3 STATIC IMPORTED)
set_target_properties(SDL3 PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libs/SDL3/lib/x64/SDL3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/libs/SDL3/include"
)

# Add ImGui
add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui PUBLIC external/imgui)
target_link_libraries(imgui PUBLIC SDL3)

# Define executable
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(AdvancedEdit ${SOURCES})
target_include_directories(AdvancedEdit PUBLIC include)
# Link libraries
target_link_libraries(AdvancedEdit SDL3 imgui)

# Include savestate files