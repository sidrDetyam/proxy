cmake_minimum_required(VERSION 3.16)
project(proxy C)

set(CMAKE_C_STANDARD 11)

file(GLOB sources src/*.c)
file(GLOB headers include/*.h)
file(GLOB utils utils/*)

add_executable(proxy
        proxy/proxy31.c
        proxy/proxy_config.h
        ${sources} ${headers} ${utils})

target_link_libraries(proxy pthread)
