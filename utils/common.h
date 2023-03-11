//
// Created by argem on 30.11.2022.
//

#ifndef PTHREAD_COMMON_H
#define PTHREAD_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

enum STATUS{
    ERROR = -1,
    SUCCESS = 0
};


#define ASSERT(cond__) \
do{\
    if(!(cond__)){ \
        perror("something goes wrong..."); \
        fprintf(stderr, #cond__);                   \
        fprintf(stderr, "\n");  \
        exit(1); \
    } \
}while(0)

#define ASSERT_RETURN(cond__) \
do{\
    if(!(cond__)){            \
        return ERROR; \
    } \
}while(0)

#define ASSERT_RETURN2(cond__, ret__) \
do{\
    if(!(cond__)){                    \
        return ret__; \
    } \
}while(0)

#define ASSERT_RETURN_C(cond__, on_fail__) \
do{\
    if(!(cond__)){                         \
        return ERROR; \
    } \
}while(0)

#define ASSERT_RETURN2_C(cond__, on_fail__, ret__) \
do{\
    if(!(cond__)){                                 \
        fprintf(stderr, "Assert: ");             \
        fprintf(stderr, #cond__);                   \
        fprintf(stderr, "\n");                     \
        on_fail__;    \
        return ret__; \
    } \
}while(0)

#define CRITICAL_M(mutex__, code__) \
do{                         \
    ASSERT(pthread_mutex_lock(&(mutex__)) == 0); \
    code__;                         \
    ASSERT(pthread_mutex_unlock(&(mutex__)) == 0);\
                            \
}while(0)

#define MIN(a__, b__) ((a__) > (b__) ? (b__) : (a__))

#define ABS(a__) ((a__) > 0? (a__) : (-(a__)))

#endif //PTHREAD_COMMON_H
