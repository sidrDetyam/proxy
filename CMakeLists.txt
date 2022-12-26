cmake_minimum_required(VERSION 3.16)
project(pthread__ C)

set(CMAKE_C_STANDARD 11)


add_executable(pthread__
        #proxy31.c
        proxy32.c
        socket_utils.h socket_utils.c common.h http_header_parser.h cvector_def.h
        cvector_impl.h http_header_parser.c connection_handler.h connection_handler.c
        hash_map.h hash_map.c)

target_link_libraries(pthread__ pthread)