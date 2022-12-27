//
// Created by argem on 07.12.2022.
//

#ifndef PTHREAD___HASH_MAP_H
#define PTHREAD___HASH_MAP_H

#include <pthread.h>

struct Pair {
    void* key;
    void* value;
};
typedef struct Pair pair_t;


#define ELEMENT_TYPE pair_t

#include "cvector_def.h"

enum{
    BUCKETS_SIZE = 1000
};

struct HashMap{
    int (*hash)(void*);
    int (*equals)(void*, void*);
    struct vpair_t** buckets;
    size_t key_size;
    size_t value_size;
    pthread_mutex_t mutex;
};
typedef struct HashMap hash_map_t;

void
hash_map_init(hash_map_t* hm, size_t ks, size_t vs, int (*hash)(void*), int (*equals)(void*, void*));

void*
hash_map_put(hash_map_t* hm, void* key, void* value);

void*
hash_map_get(hash_map_t* hm, void* key);

void
hash_map_delete(hash_map_t* hm, void* key);

void
lock(hash_map_t* hm);

void
unlock(hash_map_t* hm);

#endif //PTHREAD___HASH_MAP_H
