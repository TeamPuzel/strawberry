#ifndef BITARRAY_H
#define BITARRAY_H
#include <stddef.h>

// A resizable array of bits respecting endianness.
// If the bits fit inside the pointer itself they are stored inline.
typedef struct BitArray {
    union {
        void * allocated;
        size_t local;
    };
    size_t count;
} BitArray;

#endif
