/**
 * @file sdrad_thread.h
 * @author Merve Gulmez
 * @brief 
 * @version 0.1
 * @date 2023-05-02
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */
#include <sys/syscall.h>
#include <sys/mman.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "/usr/include/event.h"
#include <sys/resource.h> 
#include <assert.h>


#ifdef SDRAD_MULTITHREAD
#define SDRAD_MUTEX_LOCK(...)\
       sdrad_mutex_lock()

#define SDRAD_MALLOC_LOCK(...)\
       sdrad_malloc_lock()     
#else
#define SDRAD_MUTEX_LOCK(...)  
#define SDRAD_MALLOC_LOCK(...)        
#endif

#ifdef SDRAD_MULTITHREAD
#define SDRAD_MUTEX_UNLOCK(...) \
        sdrad_mutex_unlock()
#define SDRAD_MALLOC_UNLOCK(...) \
        sdrad_malloc_unlock()
#else
#define SDRAD_MUTEX_UNLOCK(...) 
#define SDRAD_MALLOC_UNLOCK(...)     
#endif

#ifdef SDRAD_MULTITHREAD
__attribute__((always_inline))
static inline int32_t sdrad_get_sti(sdrad_global_manager_t *sgm_ptr)
{
    sti_t                        tid; 
    int32_t                      total_thread; 
    int32_t                      k; 

    tid = pthread_self(); 

    total_thread = sgm_ptr -> sgm_total_thread;
    
    for(k = 0; k < total_thread+1; k++)
    {
        if(tid == sgm_ptr -> sgm_thread_id[k]){
            return k;
        }
    }
    return -1;     
}

__attribute__((always_inline))
static inline int32_t sdrad_get_sti_from_tid(sdrad_global_manager_t *sgm_ptr,  sti_t tid)
{
    int32_t                      total_thread; 
    int32_t                      k; 


    total_thread = sgm_ptr -> sgm_total_thread;
    
    for(k = 0; k < total_thread+1; k++)
    {
        if(tid == sgm_ptr -> sgm_thread_id[k]){
            return k;
        }
    }
    return -1;     
}
#else
#define sdrad_get_sti(...)       ROOT_DOMAIN;    
#endif

// mutex related functions
__attribute__((always_inline))
static inline void sdrad_mutex_lock()
{
    int32_t ret;
    ret = pthread_mutex_lock(&sdrad_global_manager.sgm_lock);
    assert(ret == 0);
    if(ret){
        switch(ret){
            case EDEADLK:
                printf( "A deadlock condition was detected \n");
                break;
            default:
                SDRADEXIT("pthread_mutex_lock");
        }
    }
}

// mutex related functions
__attribute__((always_inline))
static inline void sdrad_malloc_lock()
{
    int32_t ret;
    ret = pthread_mutex_lock(&malloc_mutex);
    assert(ret == 0);
    if(ret){
        switch(ret){
            case EDEADLK:
                printf( "A deadlock condition was detected \n");
                break;
            default:
                SDRADEXIT("pthread_mutex_lock");
        }
    }
}

__attribute__((always_inline))
static inline void sdrad_mutex_unlock()
{
    pthread_mutex_unlock(&sdrad_global_manager.sgm_lock);
}


__attribute__((always_inline))
static inline void sdrad_malloc_unlock()
{
    pthread_mutex_unlock(&malloc_mutex);
}


extern void sdrad_wait_cond(); 

extern void *sdrad_pthread_construstor();

extern void sdrad_mutex_constructor(sdrad_global_manager_t *sgm_ptr);