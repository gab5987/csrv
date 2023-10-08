# lib/CMakeLists.txt

file(GLOB CJSON_S ${CMAKE_CURRENT_SOURCE_DIR}/cJSON/*.c)
add_library(cJSON STATIC ${CJSON_S})

# Include directories if needed
target_include_directories(cJSON PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cJSON)

