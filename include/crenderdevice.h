#ifndef __CVK_DEVICE_H
#define __CVK_DEVICE_H

#ifdef __cplusplus
    extern "C" {
#endif

/*
    Renderer based 'Instance'/'Device' struct.
    I'll implement one for opengl etc when I get there.
*/

typedef struct crdevice_t crdevice_t;

crdevice_t *crdevice_init( struct SDL_Window *window );

#ifdef __cplusplus
    }
#endif

#endif//__CVK_DEVICE_H