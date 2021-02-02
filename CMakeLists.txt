cmake_minimum_required(VERSION 3.16)
project(netlearn)

set(CMAKE_CXX_STANDARD 17)
include_directories(cppcode/include)
include_directories(muduo/)

add_subdirectory(muduo/muduo/base)
add_subdirectory(muduo/muduo/net)
# cpp code
add_executable(echo_server_poll cppcode/echo_server_poll.cpp)
add_executable(simple_echo_server_one_thread_select cppcode/simple_echo_server_one_thread_select.cpp)
add_executable(simple_echo_server_one_thread_select2 cppcode/simple_echo_server_one_thread_select2.cpp)
add_executable(simple_echo_server_one_thread cppcode/simple_echo_server_one_thread.cpp)
add_executable(proxy_server_poll cppcode/tcp_relay_server_poll.cpp)

# muduo learn
add_executable(echo muduolearn/simple/echo.cpp)
add_executable(dicard muduolearn/simple/discard.cpp)
add_executable(test muduolearn/test.cpp)
add_executable(datetime muduolearn/simple/daytime.cpp)

target_link_libraries(dicard muduo_net muduo_base pthread)
target_link_libraries(echo muduo_net muduo_base pthread)
target_link_libraries(datetime muduo_net muduo_base pthread)