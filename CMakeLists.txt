cmake_minimum_required(VERSION 3.21)
project(dats_defense)

option(DRAW "Enable drawing features" OFF)

# Detect the operating system
if (WIN32)
    add_compile_options(
            "$<$<CONFIG:RELEASE>:/O2>"
            "$<$<CONFIG:RELEASE>:/W4>"
            "$<$<CONFIG:DEBUG>:/Od>"
            "$<$<CONFIG:DEBUG>:/Zi>"
    )
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif (UNIX)
    add_compile_options(
            "$<$<CONFIG:RELEASE>:-O3>"
            "$<$<CONFIG:RELEASE>:-Wall>"
            "$<$<CONFIG:RELEASE>:-Wextra>"
            "$<$<CONFIG:DEBUG>:-O0>"
    )
endif ()

# Add subdirectories
if (DRAW)
    add_subdirectory(3rdparty/clsocket)
    add_subdirectory(3rdparty/flatbuffers)
    add_subdirectory(3rdparty/rewind_viewer)
    add_definitions(-DDRAW)
endif ()

add_subdirectory(3rdparty/rapidjson)
add_subdirectory(3rdparty/loguru)
add_subdirectory(src)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)