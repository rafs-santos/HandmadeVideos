cmake_minimum_required(VERSION 3.20)

# Project name and a few useful settings. Other commands can pick up the results
project(MyGame VERSION 0.0.1)
include(cmake/StandardProjectSettings.cmake)
if(MSVC)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /FC")
endif()


# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)


# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# enable cache system
include(cmake/Cache.cmake)


# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)


add_executable(Handmade WIN32)


target_sources(
    Handmade
    PRIVATE
       "main.cpp"
    PUBLIC
)

if (MSVC)
  target_link_libraries(
    Handmade
    PRIVATE
        project_options
        project_warnings
        user32.lib
        gdi32.lib   
    )
endif ()

# target_link_libraries(
#     Tetris
#     PRIVATE
#         SDL3::SDL3
#         SDL3_ttf::SDL3_ttf
#         SDL3_image::SDL3_image
# )

