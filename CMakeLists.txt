cmake_minimum_required(VERSION 3.4.1)
PROJECT(webserver)
SET(PROJECT_ROOT_DIR)


INCLUDE_DIRECTORIES(
    /usr/include/
    ${PROJECT_ROOT_DIR}/lock
    ${PROJECT_ROOT_DIR}/mysql
)


aux_source_directory(./mysql src)
add_library(webserver src)
link_libraries(
    /usr/lib
    /usr/local/lib
    -lpthread
)

add_executable(webserver webserver.cpp)