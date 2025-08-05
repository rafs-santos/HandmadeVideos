cmake_minimum_required(VERSION 3.20)

# Project name and a few useful settings. Other commands can pick up the results
project(HandmadeHero VERSION 0.0.1)
include(cmake/StandardProjectSettings.cmake)

if(MSVC)
# Export DLL symbols at compilation to avoid needing __declspec(dllexport) in front of functions
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)
target_compile_options(project_options INTERFACE -MTd -nologo -Gm- -GR -EHa- -Od -Oi -Z7 -FC)
target_compile_definitions(project_options INTERFACE HANDMADE_INTERNAL=1 HANDMADE_SLOW=1 HANDMADE_WIN32=1)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# enable cache system
include(cmake/Cache.cmake)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

#------------------------------------------------------------------------
add_library(handmade SHARED)
target_sources(
    handmade
    PUBLIC
        "handmade.h"
        "handmade_platform.h"
        "handmade_intrinsics.h"
    PRIVATE
        "handmade.cpp"
)

target_link_libraries(
    handmade
    PRIVATE
        project_options
        project_warnings
)

# Generate unique timestamp at configuration time
# Generate a random 16-digit number for unique PDB filename
string(RANDOM LENGTH 6 ALPHABET 0123456789 RAND_SUFFIX)
# Set unique PDB path with timestamp
set(UNIQUE_PDB "handmade_${RAND_SUFFIX}.pdb")

target_link_options(handmade PRIVATE -incremental:no -opt:ref
    /MAP:${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/handmade.map
    /PDB:${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${UNIQUE_PDB})

add_custom_command(
  TARGET handmade
  PRE_BUILD
  COMMAND powershell -Command "Remove-Item -Path '\"$<TARGET_FILE_DIR:handmade>\\handmade*.pdb'"
  COMMENT "Deleting all .pdb files using PowerShell..."
)

# add_custom_command(
#     TARGET handmade
#     POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E echo "Renaming PDB file..."
#     # Generate a timestamp (this example uses PowerShell on Windows)
#     COMMAND powershell -Command "Rename-Item \"$<TARGET_FILE_DIR:handmade>\\handmade_temp.pdb\" -NewName ('handmade-' + (Get-Random) + '.pdb')"
#     COMMENT "Renaming handmade.pdb with current time stamp"
# )

# add_custom_command(TARGET handmade POST_BUILD
#                    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${IMAGE1}" "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/"
#                    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${IMAGE2}" "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/"
# )
#target_compile_definitions(handmade PRIVATE HANDMADE_INTERNAL=1 HANDMADE_SLOW=1 HANDMADE_WIN32=1)

#------------------------------------------------------------------------
add_executable(Win32Handmade WIN32)
target_sources(
    Win32Handmade
    PRIVATE 
        "win32_handmade.cpp"
    PUBLIC
        "win32_handmade.h"
)

if (MSVC)
  target_link_libraries(
    Win32Handmade
    PRIVATE
        project_options
        project_warnings
        user32.lib
        gdi32.lib
        Winmm.dll   
    )
    target_link_options(Win32Handmade PRIVATE -incremental:no -opt:ref /MAP:${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/win32_handmade.map)
    # target_compile_options(Win32Handmade PRIVATE -Fmwin32_handmade.map)
endif ()
#------------------------------------------------------------------------