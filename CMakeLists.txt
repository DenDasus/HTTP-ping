cmake_minimum_required(VERSION 2.8)
project(HTTP-ping C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(HTTP-ping main.c utils.h utils.c)
add_executable(HTTP-ping-test Test/CuTest.c Test/TestMain.c utils.h utils.c)