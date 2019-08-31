cmake_minimum_required(VERSION 3.14)
project(cstringmap C)

set(CMAKE_C_STANDARD 99)

add_executable(cstringmap main.c src/stringhm.c src/stringhm.h src/siphash.c src/siphash.h)