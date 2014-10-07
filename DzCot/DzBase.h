/**
 *  @file       DzBase.h
 *  @brief      
 *  @author     Foreverflying <foreverflying@live.cn>
 *  @date       2011/10/24
 *
 */

#ifndef __DzBase_h__
#define __DzBase_h__

#include "DzInc.h"
#include "DzStructs.h"
#include "DzBaseOs.h"

#ifdef __cplusplus
extern "C"{
#endif

inline DzLItr* AtomPopStack( DzLItr** top )
{
    DzLItr* now;
    DzLItr* origin;

    origin = NULL;
    now = (DzLItr*)AtomReadPtr( (void**)top );
    while( now && now != origin ){
        origin = now;
        now = (DzLItr*)AtomCasPtr( (void**)top, origin, origin->next );
    }
    return now;
}

inline void AtomPushStack( DzLItr** top, DzLItr* lItr )
{
    DzLItr* now;
    DzLItr* origin;

    origin = NULL;
    now = (DzLItr*)AtomReadPtr( (void**)top );
    do{
        lItr->next = now;
        origin = now;
        now = (DzLItr*)AtomCasPtr( (void**)top, origin, lItr );
    }while( now != origin );
}

#ifdef __cplusplus
};
#endif

#endif // __DzBase_h__
