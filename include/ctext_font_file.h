#ifndef __CTEXT_FF_H
#define __CTEXT_FF_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct cfont_t cfont_t;

// format of .ff
// { ctext_ff_header }
// { ctext_glyph_entry } glyphs

typedef struct ctext_ff_header {
    // The path is stored so we may easily check if this is the one we're looking for
    // ctext will look at the path, and if it is the same that the user is looking at, it will just load that instead.
    // 2nd check will be for the family and style names because I'm sure I'll name a font file piss.ff twice
    char path[ 128 ];
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