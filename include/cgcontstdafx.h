#ifndef __CG_CONT_STDAFX_H
#define __CG_CONT_STDAFX_H

// Maybe they should be static variables, idk.

#ifndef cg_cont_alloc
    #define cg_cont_alloc malloc
#endif

#ifndef cg_cont_calloc
    #define cg_cont_calloc calloc
#endif

#ifndef cg_cont_realloc
    #define cg_cont_realloc realloc
#endif

#ifndef cg_cont_free
    #define cg_cont_free free
#endif

#endif//__CG_CONT_STDAFX_H