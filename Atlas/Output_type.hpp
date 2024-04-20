#ifndef __ATLAS_OUTPUT_TYPE__
#define __ATLAS_OUTPUT_TYPE__

struct Atlas_Vec2 {
    const float x, y;
};

struct Atlas_OutputImage {
    const char* name;
    const Atlas_Vec2 texturePositions[4];
};

constexpr bool constexpr_strcmp(const char* a, const char* b) {
    while (*a || *b){
        if (*a++ != *b++)
            return false;
    }
    return true;
}

#endif