cmake_minimum_required(VERSION 3.28)
project(bfa)

set(CMAKE_C_STANDARD 90)

add_executable(bfa main.c)
target_link_libraries(bfa PRIVATE pcap)