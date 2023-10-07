cmake_minimum_required(VERSION 3.10)

project(csrv)

set(CMAKE_C_STANDARD 11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)

file(GLOB SOURCES src/*.c)

find_package (mongoc-1.0 1.7 REQUIRED)

add_executable(csrv ${SOURCES})
target_compile_options(csrv PRIVATE -g)
set_target_properties(csrv PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
target_link_libraries (csrv PRIVATE mongo::mongoc_shared)
target_link_libraries(csrv PRIVATE pthread)

install(TARGETS ${OBJECT_FILES}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

file(GLOB HEADER_FILES include/*.h)
install(FILES ${HEADER_FILES} DESTINATION include)

install(TARGETS ${OBJECT_FILES}
    RUNTIME DESTINATION /usr/local/bin
)
