cmake_minimum_required(VERSION 2.8.12)
project(chat_room_server)

add_definitions("-std=c++14")

aux_source_directory(./src SRC)

include(${PROJECT_SOURCE_DIR}/build/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS} mysqlclient)
