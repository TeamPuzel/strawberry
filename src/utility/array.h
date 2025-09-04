#ifndef ARRAY_H
#define ARRAY_H
#include <stddef.h>
#include "allocator.h" // IWYU pragma: keep // This is needed indirectly by the array definition macro.

#define DEFINE_ARRAY(T)                                                                                                \
typedef struct T##Array {                                                                                              \
    T * data;                                                                                                          \
    size_t size;                                                                                                       \
    size_t count;                                                                                                      \
} T##Array;                                                                                                            \
static inline void T##Array_append(T##Array * self, Allocator * alloc, T value) {                                      \
    if (self->count == self->size) self->data = Allocator_realloc(alloc, self->data, self->size * 2 * sizeof(T));      \
    self->data[self->count] = value;                                                                                   \
    self->count += 1;                                                                                                  \
}                                                                                                                      \
static inline void T##Array_free(T##Array * self, Allocator * alloc) {                                                 \
    alloc->free(alloc, self->data);                                                                                    \
}

#define ARRAY_CAST(T, ARRAY) (T##Array) {                                                                              \
    .data = (T *) ARRAY.data,                                                                                          \
    .size = ARRAY.size,                                                                                                \
    .count = ARRAY.count                                                                                               \
}

#endif
