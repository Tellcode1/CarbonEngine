#ifndef __CTEXT_FF_H
#define __CTEXT_FF_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct cfont_t cfont_t;

// format of .ff
// { ctext_ff_header }
// { ctext_glyph_entry } glyphs
// bitmap | make format for this? like add serialization api to catlas?

typedef struct ctext_ff_header {
    char family_name[ 128 ];
    char style_name[ 128 ];
    float line_height,space_width;
    int bmpwidth, bmpheight;
    int numglyphs;
} ctext_ff_header;

typedef struct ctext_glyph_entry {
    unsigned codepoint;
    float advance;
    float x0,x1,y0,y1;
    float l,b,r,t;
} ctext_glyph_entry;

extern void ctext_ff_write(const char *path, const cfont_t *fnt);

/*
    returns 0 on success, 1 on failure.
*/
extern int ctext_ff_read(const char *path, cfont_t *fnt);

#ifdef __cplusplus
    }
#endif

#endif//__CTEXT_FF_H