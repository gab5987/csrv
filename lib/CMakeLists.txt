# lib/CMakeLists.txt

file(GLOB CJSON_S cJSON/*.c)
file(GLOB CJSON_H cJSON/*.h)
add_library(cJSON STATIC ${CJSON_S} ${CJSON_H})

# Include directories if needed
# target_include_directories(cJSON PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

