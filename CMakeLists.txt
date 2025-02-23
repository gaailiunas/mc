cmake_minimum_required(VERSION 3.20)
project(mc-server)
add_executable(mc-server src/main.c src/server.c)
target_compile_options(mc-server PRIVATE -Wall -Wextra -Wpedantic)
