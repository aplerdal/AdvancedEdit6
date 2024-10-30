cmake_minimum_required(VERSION 3.22)
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
list(APPEND CMAKE_PREFIX_PATH "SDL3/cmake/")
  find_package(SDL3 REQUIRED)


# Add ImGui
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui PUBLIC imgui)
target_link_libraries(imgui PRIVATE ${SDL3_LIBRARIES})

# Define executable
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(AdvancedEdit ${SOURCES})

target_include_directories(AdvancedEdit PUBLIC include)
# Link libraries
target_link_libraries(AdvancedEdit PRIVATE ${SDL3_LIBRARIES} imgui)

if(WIN32)
    add_custom_command(
        TARGET AdvancedEdit POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:AdvancedEdit>
        VERBATIM
    )
endif()