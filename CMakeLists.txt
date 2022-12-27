cmake_minimum_required(VERSION 3.16)
project(proxy C)

set(CMAKE_C_STANDARD 11)


add_executable(proxy
        #proxy31.c
        proxy32.c
        proxy_config.h
        socket_utils.h socket_utils.c common.h http_utils.h cvector_def.h
        cvector_impl.h http_utils.c connection_handler.h connection_handler.c
        hash_map.h hash_map.c)

target_link_libraries(proxy pthread)