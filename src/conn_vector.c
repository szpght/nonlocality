#include <stdlib.h>
#include "nonlocality.h"

void vector_init(ConnectionVector *vector) {
    vector->capacity = 8;
    vector->size = 0;
    vector->conns = malloc (sizeof(ConnectionPair) * vector->capacity);
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