#ifndef ARRAY_H
#define ARRAY_H
#include <stddef.h>
#include "allocator.h"

#define DECLARE_ARRAY(T)                                                                                               \
typedef struct T##Array {                                                                                              \
    T * data;                                                                                                          \
    size_t size;                                                                                                       \
    size_t count;                                                                                                      \
} T##Array;

#define IMPLEMENT_ARRAY(T)                                                                                             \
static inline void T##Array_append(T##Array * self, Allocator * alloc, T value) {                                      \
    if (self->count == self->size) {                                                                                   \
        size_t new_size = (self->size == 0) ? 10 : self->size * 2;                                                     \
        self->data = Allocator_realloc(alloc, self->data, new_size * sizeof(T));                                       \
        self->size = new_size;                                                                                         \
    }                                                                                                                  \
    self->data[self->count] = value;                                                                                   \
    self->count += 1;                                                                                                  \
}                                                                                                                      \
static inline void T##Array_free(T##Array * self, Allocator * alloc) {                                                 \
    alloc->free(alloc, self->data);                                                                                    \
}                                                                                                                      \
static inline void T##Array_clear(T##Array * self) {                                                                   \
    self->count = 0;                                                                                                   \
}

#define ARRAY_CAST(T, ARRAY) (T##Array) {                                                                              \
    .data = (T *) ARRAY.data,                                                                                          \
    .size = ARRAY.size,                                                                                                \
    .count = ARRAY.count                                                                                               \
}

#define DEFINE_ARRAY(T) DECLARE_ARRAY(T) IMPLEMENT_ARRAY(T)

#endif
