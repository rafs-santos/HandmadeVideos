cmake_minimum_required(VERSION 3.20)

# Project name and a few useful settings. Other commands can pick up the results
project(MyGame VERSION 0.0.1)
include(cmake/StandardProjectSettings.cmake)
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELEASE} /MT /nologo /GR- /EHa /Od /Oi /Z7 /FC /Gm")

# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /opt:ref /SUBSYSTEM:WINDOWS,5.1 
#                             /MAP:${PROJECT_BINARY_DIR}/win32_handmade.map")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /opt:ref
                            /MAP:${PROJECT_BINARY_DIR}/win32_handmade.map")
endif()


# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)


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
        "win32_handmade.cpp"
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

target_compile_definitions(Handmade PRIVATE HANDMADE_INTERNAL=1 HANDMADE_SLOW=1 HANDMADE_WIN32=1)

# target_link_libraries(
#     Tetris
#     PRIVATE
#         SDL3::SDL3
#         SDL3_ttf::SDL3_ttf
#         SDL3_image::SDL3_image
# )

