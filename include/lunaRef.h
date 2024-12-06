#ifndef __LUNA_REF_H
#define __LUNA_REF_H

#include <stdbool.h>

typedef struct luna_Ref {
    int count; // reference count;
} luna_Ref;

luna_Ref luna_Ref_Init() {
    return (luna_Ref) {
        .count = 1
    };
}

void luna_Ref_Increment(luna_Ref *ref) {
    ref->count++;
}

// Returns 1 if the reference counter reaches 0
// You may use that to destroy the structure when it is no longer being used.
bool luna_Ref_Decrement(luna_Ref *ref) {
    ref->count--;
    if (ref->count <= 0) {
        return 1;
    }
    return 0;
}

#endif//__LUNA_REF_H