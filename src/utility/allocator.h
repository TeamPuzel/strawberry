#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <stddef.h>
#include <stdlib.h>

// Allocator interface -------------------------------------------------------------------------------------------------

// Simple polymorphic allocator interface.
typedef struct Allocator {
    void * (*alloc)(void * self, size_t size);
    void * (*realloc)(void * self, void * ptr, size_t size);
    void (*free)(void * self, void * ptr);
} Allocator;

static inline void * Allocator_alloc(void * self, size_t size) {
    return ((Allocator *) self)->alloc(self, size);
}

static inline void * Allocator_realloc(void * self, void * ptr, size_t size) {
    return ((Allocator *) self)->realloc(self, ptr, size);
}

static inline void Allocator_free(void * self, void * ptr) {
    ((Allocator *) self)->free(self, ptr);
}

// Default C allocator -------------------------------------------------------------------------------------------------

static inline void * c_alloc_alloc(void * self, size_t size) {
    return malloc(size);
}

static inline void * c_alloc_realloc(void * self, void * ptr, size_t size) {
    return realloc(ptr, size);
}

static inline void c_alloc_free(void * self, void * ptr) {
    free(ptr);
}

static inline Allocator c_alloc() {
    return (Allocator) {
        .alloc = c_alloc_alloc,
        .free = c_alloc_free
    };
}

// Arena allocator -----------------------------------------------------------------------------------------------------

// typedef struct ArenaAllocator {
//     Allocator super;
//     Allocator * inner;
// } ArenaAllocator;

// static inline ArenaAllocator arena_alloc(Allocator * inner) {

// }

#endif
