# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources} REQUIRES esp_wifi esp_http_client esp_event esp_timer driver soc esp_driver_tsens lwip EMBED_FILES ../webpage.bin)

