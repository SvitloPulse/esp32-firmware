cmake_minimum_required(VERSION 3.16.0)
set(PROJECT_NAME "SvitloPulse")
set(PROJECT_NAME_LOWER "")
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
set(PROJECT_VER "$ENV{APPVEYOR_REPO_TAG_NAME}")
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")
add_compile_definitions(PROJECT_VER="${PROJECT_VER}")
add_compile_definitions(PROJECT_NAME_LOWER="${PROJECT_NAME_LOWER}")
file(DOWNLOAD "https://github.com/SvitloPulse/embedded-web-ui/releases/latest/download/webpage.gz" "${CMAKE_SOURCE_DIR}/webpage.bin")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(svitlopulse)
