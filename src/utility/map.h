#ifndef MAP_H
#define MAP_H

#define DEFINE_MAP(K, V)                                                                                               \
typedef struct K##V##Bucket {                                                                                          \
    V##Array values;                                                                                                   \
} K##V##Bucket;                                                                                                        \
typedef struct K##V##Map {                                                                                             \
    K##V##BucketArray                                                                                                  \
} K##V##Map;

#endif
