
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<assert.h>


#define CONCAT(a,b) CONCAT_V(a,b)
#define CONCAT_V(a,b) a##b

#define VECTOR CONCAT(v, ELEMENT_TYPE)


struct VECTOR{
    ELEMENT_TYPE* ptr;
    size_t cnt;
    size_t capacity;
};
typedef struct VECTOR VECTOR;


ELEMENT_TYPE*
CONCAT(VECTOR, _get)(struct VECTOR* vector, size_t ind);

void
CONCAT(VECTOR, _assign) (struct VECTOR* vector, ELEMENT_TYPE* el, size_t ind);

ELEMENT_TYPE*
CONCAT(VECTOR, _back) (struct VECTOR* vector);

void
CONCAT(VECTOR, _forced_alloc) (struct VECTOR* vector);

void
CONCAT(VECTOR, _alloc) (struct VECTOR* vector);

void
CONCAT(VECTOR, _alloc2) (struct VECTOR* vector, size_t cnt);

void
CONCAT(VECTOR, _push_back) (struct VECTOR* vector, ELEMENT_TYPE* el);

void
CONCAT(VECTOR, _pop_back) (struct VECTOR* vector);

void
CONCAT(VECTOR, _truncate) (struct VECTOR* vector);

void
CONCAT(VECTOR, _init) (struct VECTOR* vector);

void
CONCAT(VECTOR, _free) (struct VECTOR* vector);

void
CONCAT(VECTOR, _remove) (struct VECTOR* vector, size_t ind);

void
CONCAT(VECTOR, _free_ptr) (struct VECTOR* vector);

void
CONCAT(VECTOR, _clear) (struct VECTOR* vector);

#undef ELEMENT_TYPE
#undef CONCAT
#undef CONCAT_V
#undef VECTOR
