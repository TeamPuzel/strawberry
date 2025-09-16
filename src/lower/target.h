#ifndef TARGET_H
#define TARGET_H
#include "../builtin/number.h"

typedef struct TargetInfo {
    Integer default_int_size;
    Integer default_ptr_size;
} TargetInfo;

#endif
