
#ifndef REALLOC_RATIO
#define REALLOC_RATIO 2
#endif


__attribute__((unused)) ELEMENT_TYPE *
CONCAT(VECTOR, _get)(struct VECTOR *vector, size_t ind) {
    //assert("Out of range" && vector->cnt <= ind);
    return vector->ptr + ind;
}


void
CONCAT(VECTOR, _assign)(struct VECTOR *vector, ELEMENT_TYPE *el, size_t ind) {
    //assert(vector->cnt <= ind && el!=NULL);
    memcpy(vector->ptr + ind, el, sizeof(ELEMENT_TYPE));
}


ELEMENT_TYPE *
CONCAT(VECTOR, _back)(struct VECTOR *vector) {
    return vector->ptr + vector->cnt - 1;
}


void
CONCAT(VECTOR, _forced_alloc)(struct VECTOR *vector) {
    void *tmp = realloc(vector->ptr,
                        (vector->capacity * REALLOC_RATIO + 1) * sizeof(ELEMENT_TYPE));
    if (tmp == NULL) {
        perror("Out of memory");
        exit(1);
    }

    vector->capacity = vector->capacity * REALLOC_RATIO + 1;
    vector->ptr = tmp;
}


void
CONCAT(VECTOR, _alloc)(struct VECTOR *vector) {
    if (vector->cnt == vector->capacity) {
        CONCAT(VECTOR, _forced_alloc)(vector);
    }
}


void
CONCAT(VECTOR, _alloc2)(struct VECTOR *vector, size_t cnt) {
    while (vector->capacity - vector->cnt < cnt) {
        CONCAT(VECTOR, _forced_alloc)(vector);
    }
}


void
CONCAT(VECTOR, _push_back)(struct VECTOR *vector, ELEMENT_TYPE *el) {
    CONCAT(VECTOR, _alloc)(vector);
    memcpy(&vector->ptr[vector->cnt], el, sizeof(ELEMENT_TYPE));
    ++vector->cnt;
}


void
CONCAT(VECTOR, _pop_back)(struct VECTOR *vector) {
    --vector->cnt;
}

void
CONCAT(VECTOR, _remove) (struct VECTOR* vector, size_t ind){
    for(size_t i = ind; i < vector->cnt; ++i){
        memcpy(&vector->ptr[i], &vector->ptr[i+1], sizeof(ELEMENT_TYPE));
    }
    --vector->cnt;
}


void
CONCAT(VECTOR, _init)(struct VECTOR *vector) {
    vector->cnt = 0;
    vector->ptr = NULL;
    vector->capacity = 0;
}


void
CONCAT(VECTOR, _free)(struct VECTOR *vector) {
    free(vector->ptr);
    CONCAT(VECTOR, _init)(vector);
}


void
CONCAT(VECTOR, _free_ptr)(struct VECTOR *vector) {
    int *ptr;
    for (size_t i = 0; i < vector->cnt; ++i) {
        memcpy(&ptr, &vector->ptr[i], sizeof(int *));
        free(ptr);
    }
    CONCAT(VECTOR, _free)(vector);
}

#undef REALLOC_RATIO
#undef ELEMENT_TYPE
