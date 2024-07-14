set(APP_NAME "mortido-bot")

file(GLOB_RECURSE SOURCE_FILES "**.cpp")
file(GLOB_RECURSE HEADER_FILES "**.h")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
add_executable(${APP_NAME} ${SOURCE_FILES} ${HEADER_FILES} api/responses.h api/requests.h models/zombie.h models/player.h models/map.h models/vec2d.h)
target_compile_features(${APP_NAME} PUBLIC cxx_std_20)

include(FindPkgConfig)
pkg_check_modules(CURLPP REQUIRED curlpp)

# Debug information
message(STATUS "CURLPP_INCLUDE_DIRS: ${CURLPP_INCLUDE_DIRS}")
message(STATUS "CURLPP_LDFLAGS: ${CURLPP_LDFLAGS}")

target_link_libraries(${APP_NAME} PRIVATE rapidjson loguru ${CURLPP_LDFLAGS})
#target_include_directories(${APP_NAME} PUBLIC ${CURLPP_INCLUDE_DIRS})
target_include_directories(${APP_NAME} PRIVATE ${CURLPP_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

if (DRAW)
    target_link_libraries(${APP_NAME} PRIVATE rewind_viewer_client)
endif ()
