#include <stdlib.h>
#include "nonlocality.h"

void vector_init(ConnectionVector *vector) {
    vector->capacity = 8;
    vector->size = 0;
    vector->conns = malloc (sizeof(ConnectionPair) * vector->capacity);

    // setting mutex to recursive in case of signal
    // being handled by thread already owning mutex
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&vector->mutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);
}


void vector_add(ConnectionVector *vector, ConnectionPair pair) {
    if (vector->size == vector->capacity) {
        vector->capacity *= 2;
        vector->conns = realloc(vector->conns, sizeof(ConnectionPair) * vector->capacity);
    }
    vector->conns[vector->size] = pair;
    ++vector->size;
}


void vector_delete(ConnectionVector *vector, int position) {
    for (int i = position + 1; i < vector->size; ++i)
        vector->conns[i - 1] = vector->conns[i];
    --vector->size;
}