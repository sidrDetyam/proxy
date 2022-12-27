//
// Created by argem on 07.12.2022.
//

#include "hash_map.h"
#include "common.h"

#define ELEMENT_TYPE pair_t
#include "cvector_impl.h"

void
hash_map_init(hash_map_t* hm, size_t ks, size_t vs, int (*hash)(void*), int (*equals)(void*, void*)){
    hm->key_size = ks;
    hm->value_size = vs;
    hm->equals = equals;
    hm->hash = hash;
    hm->buckets = malloc(sizeof(void *) * BUCKETS_SIZE);
    ASSERT(hm->buckets != NULL);
    memset(hm->buckets, 0, sizeof(void *) * BUCKETS_SIZE);

    pthread_mutexattr_t attr;
    ASSERT(pthread_mutexattr_init(&attr) == 0);
    ASSERT(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) == 0);
    ASSERT(pthread_mutex_init(&hm->mutex, &attr) == 0);
}

static vpair_t *
find_bucket_or_create(hash_map_t* hm, void* key){
    int hash = hm->hash(key);
    size_t ind = ABS(hash) % BUCKETS_SIZE;
    if(hm->buckets[ind] == NULL){
        hm->buckets[ind] = malloc(sizeof(vpair_t));
        vpair_t_init(hm->buckets[ind]);
    }
    return hm->buckets[ind];
}


static void
pair_set(hash_map_t *hm, pair_t* pair, void* key, void* value){
    memcpy(pair->key, key, hm->key_size);
    memcpy(pair->value, value, hm->value_size);
}


static void*
pair_reset(hash_map_t *hm, pair_t* pair, void* key, void* value){
    void* old_value = pair->value;
    pair_set(hm, pair, key, value);
    return old_value;
}


static void
pair_init(hash_map_t *hm, pair_t* pair, void* key, void* value){
    pair->key = malloc(hm->key_size);
    pair->value = malloc(hm->value_size);
    ASSERT(pair->key != NULL && pair->value != NULL);
    pair_set(hm, pair, key, value);
}


__attribute__((unused)) static void
pair_destroy(pair_t* pair){
    free(pair->key);
    free(pair->value);
}


void*
hash_map_put(hash_map_t* hm, void* key, void* value){
    ASSERT(pthread_mutex_lock(&hm->mutex) == 0);
    vpair_t* bucket = find_bucket_or_create(hm, key);

    for(size_t i=0; i<bucket->cnt; ++i){
        pair_t* p = vpair_t_get(bucket, i);
        if(hm->equals(p->key, key)){
            void* old_value = pair_reset(hm, p, key, value);
            ASSERT(pthread_mutex_unlock(&hm->mutex) == 0);
            return old_value;
        }
    }
    pair_t pair;
    pair_init(hm, &pair, key, value);
    vpair_t_push_back(bucket, &pair);
    ASSERT(pthread_mutex_unlock(&hm->mutex) == 0);
    return NULL;
}


void*
hash_map_get(hash_map_t* hm, void* key){
    ASSERT(pthread_mutex_lock(&hm->mutex) == 0);
    vpair_t* bucket = find_bucket_or_create(hm, key);

    for(size_t i=0; i<bucket->cnt; ++i){
        pair_t* p = vpair_t_get(bucket, i);
        if(hm->equals(p->key, key)){
            ASSERT(pthread_mutex_unlock(&hm->mutex) == 0);
            return p->value;
        }
    }
    ASSERT(pthread_mutex_unlock(&hm->mutex) == 0);
    return NULL;
}


void
hash_map_delete(hash_map_t* hm, void* key){
    ASSERT(pthread_mutex_lock(&hm->mutex) == 0);
    vpair_t* bucket = find_bucket_or_create(hm, key);

    for(size_t i=0; i<bucket->cnt; ++i){
        pair_t* p = vpair_t_get(bucket, i);
        if(hm->equals(p->key, key)){
            vpair_t_remove(bucket, i);
            ASSERT(pthread_mutex_unlock(&hm->mutex) == 0);
            return;
        }
    }
    ASSERT(pthread_mutex_unlock(&hm->mutex) == 0);
}


void
lock(hash_map_t* hm){
    ASSERT(pthread_mutex_lock(&hm->mutex)==0);
}

void
unlock(hash_map_t* hm){
    ASSERT(pthread_mutex_unlock(&hm->mutex)==0);
}