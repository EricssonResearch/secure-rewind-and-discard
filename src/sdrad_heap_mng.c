/**
 * @file sdrad_heap_mng.c
 * @author Merve Gulmez 
 * @brief This folder contains the memory management of SDRaD
 * @version 0.1
 * @date 2022-02-07
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <assert.h>
#include "stdbool.h"
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
#include <string.h>
#include <pthread.h>
#include <sys/resource.h> 
#include "sdrad.h"
#include "sdrad_pkey.h"
#include "sdrad_thread.h"
#include "tlsf.h" 
#include "sdrad_heap_mng.h"
#include "sdrad_api.h"


__thread tlsf_t tlsf_structure; 


/* Wrapping glibc realloc with tlsf version */ 
void *realloc(void *ptr, size_t size)
{
    sdrad_thread_metadata_t  *stm_ptr; 
    sdrad_d_info_t           *sdi_ptr;
    sdrad_global_manager_t   *sgm_ptr;
    int32_t                  active_domain; 

   if (tlsf_structure == NULL){
        sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain

        sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;
    
        //Get STI from TDI 
        int32_t thread_index = sdrad_get_sti(sgm_ptr); 
        stm_ptr = sgm_ptr -> stm_ptr[thread_index];
    
        // Get the active domain 
        active_domain = stm_ptr -> active_domain; 
        sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[active_domain]; 
        sdrad_heap_domain_init(sdi_ptr, sgm_ptr-> pdi[active_domain]); 
        tlsf_structure = sdi_ptr -> sdi_tlsf; 
        sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]);
    }
    ptr = tlsf_realloc(tlsf_structure, ptr, size);
    return ptr;
}



void free(void *ptr)
{
    sdrad_d_info_t              *sdi_ptr;
    sdrad_global_manager_t      *sgm_ptr;
    sdrad_thread_metadata_t     *stm_ptr; 
    int32_t                     active_domain;

   if (tlsf_structure == NULL){
        sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain

        sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;
    
        //Get STI from TDI 
        int32_t thread_index = sdrad_get_sti(sgm_ptr); 
        stm_ptr = sgm_ptr -> stm_ptr[thread_index];
    
        // Get the active domain 
        active_domain = stm_ptr -> active_domain; 
        sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[active_domain]; 
        sdrad_heap_domain_init(sdi_ptr, sgm_ptr-> pdi[active_domain]); 
        tlsf_structure = sdi_ptr -> sdi_tlsf; 
        sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]);
    }
    tlsf_free(tlsf_structure, ptr);
}

int posix_memalign(void **memptr, size_t alignment, size_t size)
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_thread_metadata_t      *stm_ptr, *stm_parent_ptr; 
    sdrad_d_info_t               *sdi_ptr, *shm_parent_ptr;
    int32_t                      parent_domain_sti;
    void                        *ptr; 
    int32_t                      sdi; 
    int32_t                      active_domain;

   if (tlsf_structure == NULL){
        sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain

        sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;
    
        //Get STI from TDI 
        int32_t thread_index = sdrad_get_sti(sgm_ptr); 
        stm_ptr = sgm_ptr -> stm_ptr[thread_index];
    
        // Get the active domain 
        active_domain = stm_ptr -> active_domain; 
        sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[active_domain]; 
        sdrad_heap_domain_init(sdi_ptr, sgm_ptr-> pdi[active_domain]); 
        tlsf_structure = sdi_ptr -> sdi_tlsf; 
        sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]);
    }
    
    ptr = tlsf_memalign(tlsf_structure, alignment, size); 

    *memptr = ptr; 
    return 0; //tlsf_memalign doesn't have any zero code
}

/*
void *malloc(size_t size);
void *malloc(size_t size)
{
    sdrad_thread_metadata_t  *stm_ptr; 
    sdrad_d_info_t           *sdi_ptr;
    sdrad_global_manager_t   *sgm_ptr;
    int32_t                  active_domain; 
    void                     *ptr; 
    void                     *mmap_new_area;

    sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;
    
    //Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];
    
    // Get the active domain 
    active_domain = stm_ptr -> active_domain; 

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[active_domain]; 
 
    if (sdi_ptr -> sdi_init_heap_flag == false){
        sdrad_heap_domain_init(sdi_ptr, sgm_ptr-> pdi[active_domain]); 
    }

    ptr = tlsf_malloc(sdi_ptr -> sdi_tlsf, size);

    // return to old pkru value 
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return ptr;  
}
*/

void* (*real_malloc)(size_t) = NULL;
void *malloc(size_t size);
void *malloc(size_t size)
{
    sdrad_thread_metadata_t  *stm_ptr; 
    sdrad_d_info_t           *sdi_ptr;
    sdrad_global_manager_t   *sgm_ptr;
    sdrad_monitor_domain_t    *smd_ptr;
    int32_t                  active_domain; 
    void                     *ptr; 
    void                     *mmap_new_area;

    if (tlsf_structure == NULL){ // RUST IMPLEMENTATION
        sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain

        sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;

        if(sdrad_constructor_flag == false){
            smd_ptr = (sdrad_monitor_domain_t *)&sgm_ptr -> sdrad_monitor_domain; 
 
             /* Create  Monitor Data Domain */
            smd_ptr -> smd_monitor_sdi = sdrad_create_monitor_domain();
            /* Mapping TID (Thread ID) to STI (SDRoB Thread Index). */
            sgm_ptr -> sgm_thread_id[ROOT_DOMAIN] = pthread_self(); 
            sgm_ptr -> sgm_sdi_to_tdi[ROOT_DOMAIN] = ROOT_DOMAIN;

            sgm_ptr -> sgm_total_thread = ROOT_DOMAIN;

            sgm_ptr -> stm_ptr[sgm_ptr -> sgm_total_thread] = sdrad_malloc_mdd(sizeof(sdrad_thread_metadata_t)); 
            stm_ptr = sgm_ptr -> stm_ptr[sgm_ptr -> sgm_total_thread]; 
            stm_ptr -> active_domain = ROOT_DOMAIN;
            sdrad_constructor_flag = true; 
        }  

        //Get STI from TDI 
        int32_t thread_index = sdrad_get_sti(sgm_ptr); 
        stm_ptr = sgm_ptr -> stm_ptr[thread_index];
    
        // Get the active domain 
        active_domain = stm_ptr -> active_domain; 
        sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[active_domain]; 

        sdrad_heap_domain_init(sdi_ptr, sgm_ptr-> pdi[active_domain]); 

        tlsf_structure = sdi_ptr -> sdi_tlsf; 
        
        sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    }
    ptr = tlsf_malloc(tlsf_structure, size);
    return ptr;   
}


void *__malloc(size_t size)
{
    return malloc(size); 
}

char *strdup (const char *s)
{
    size_t len = strlen (s) + 1;
    void *new = __malloc (len);
    if (new == NULL){
        return NULL;
    }
    return (char *) memcpy (new, s, len);
}



void *calloc (size_t nelem, size_t elsize)
{
    register void *ptr;  
    if (nelem == 0 || elsize == 0)
        nelem = elsize = 1;
    
    ptr = __malloc(nelem * elsize); //sdradmalloc
    if (ptr) bzero (ptr, nelem * elsize);
    
    return ptr;
} 


/* Memory management related API */ 

void sdrad_free(udi_t udi, void *ptr)
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_thread_metadata_t      *stm_ptr, *stm_parent_ptr;
    sdrad_d_info_t               *sdi_ptr, *shm_parent_ptr;
    tlsf_t    *tlsf;
    int32_t    parent_domain_sti;
    int32_t    sdi; 

    sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    // Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];
    SDRAD_MUTEX_LOCK(); 
#ifdef SDRAD_MULTITHREAD
    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head);
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi];
    if (sdi_ptr -> sdi_init_heap_flag == false ){
        parent_domain_sti = sgm_ptr -> sgm_sdi_to_tdi[sdi]; 
        if(parent_domain_sti != thread_index){
            stm_parent_ptr = sgm_ptr -> stm_ptr[parent_domain_sti]; // get parent thread 
            shm_parent_ptr = (sdrad_d_info_t *)&stm_parent_ptr -> sdrad_d_info[sdi];  // parent domain info
            memcpy(sdi_ptr, shm_parent_ptr, sizeof(sdrad_d_info_t));
            sdi_ptr -> sdi_init_heap_flag = true;
                
        }
    }
              
    tlsf = sdi_ptr -> sdi_tlsf; 
#else 
    tlsf = (tlsf_t *)sdrad_search_tlsf_control(ptr, sgm_ptr);
#endif

    tlsf_free(tlsf, ptr); 
    SDRAD_MUTEX_UNLOCK(); 
    sdrad_store_pkru_config(stm_ptr-> pkru_config[stm_ptr-> active_domain]); 
}




void *sdrad_realloc(udi_t udi, void *ptr, size_t size)
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_thread_metadata_t      *stm_ptr, *stm_parent_ptr;
    sdrad_d_info_t        *sdi_ptr, *shm_parent_ptr; 
    int32_t    sdi; 
    sdrad_domain_t  active_domain; 
    int32_t    parent_domain_sti;

    sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    
    // Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head);
    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP){
        assert(sdi != SDRAD_ERROR_DOMAIN_NO_MAP);
        return (void *)SDRAD_ERROR_NO_DOMAIN; 
    }
    
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi]; 
    SDRAD_MUTEX_LOCK();
    if (sdi_ptr -> sdi_init_heap_flag == false ){
        parent_domain_sti = sgm_ptr -> sgm_sdi_to_tdi[sdi]; 
        if(parent_domain_sti != thread_index){
            stm_parent_ptr = sgm_ptr -> stm_ptr[parent_domain_sti]; // get parent thread 
            shm_parent_ptr = (sdrad_d_info_t *)&stm_parent_ptr -> sdrad_d_info[sdi];  // parent domain info
            memcpy(sdi_ptr, shm_parent_ptr, sizeof(sdrad_d_info_t));
            sdi_ptr -> sdi_init_heap_flag = true;
            if(shm_parent_ptr -> sdi_init_heap_flag == false){
                sdrad_heap_domain_init(shm_parent_ptr, sgm_ptr-> pdi[sdi]);
            }
        }else{
            sdrad_heap_domain_init(sdi_ptr, sgm_ptr-> pdi[sdi]);
        }
    }

    ptr = tlsf_realloc(sdi_ptr -> sdi_tlsf, ptr, size);
    SDRAD_MUTEX_UNLOCK(); 
    active_domain = stm_ptr -> active_domain;
    // return to old pkru value
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return ptr;   
}


int sdrad_memalign(udi_t udi, void **memptr, size_t alignment, size_t size)
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_thread_metadata_t      *stm_ptr, *stm_parent_ptr;
    sdrad_d_info_t               *sdi_ptr, *shm_parent_ptr; 
    sdrad_domain_t  active_domain; 
    void                        *ptr; 
    int32_t    sdi; 
    int32_t    parent_domain_sti;

    sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    
    // Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head);
    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP){
        assert(sdi != SDRAD_ERROR_DOMAIN_NO_MAP);
        return SDRAD_ERROR_NO_DOMAIN; 
    }
    
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi]; 
    SDRAD_MALLOC_LOCK();
    if (sdi_ptr -> sdi_init_heap_flag == false){
        sdrad_heap_domain_init(sdi_ptr, sgm_ptr-> pdi[sdi]); 
    }
    
    ptr = tlsf_memalign(sdi_ptr -> sdi_tlsf, alignment, size); 
    SDRAD_MALLOC_UNLOCK();
    *memptr = ptr; 
    // return to old pkru value
    active_domain = stm_ptr -> active_domain;
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return 0; //tlsf_memalign doesn't have any zero code
}




void *sdrad_malloc(udi_t udi, size_t size)
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_thread_metadata_t      *stm_ptr, *stm_parent_ptr; 
    sdrad_d_info_t               *sdi_ptr, *shm_parent_ptr;
    udi_t                      parent_domain_sti;
    void                        *ptr; 
    int32_t                      sdi; 
    sdrad_domain_t  active_domain; 

    sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    
    // Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    assert(thread_index != -1);
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head);
    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP){
        assert(sdi != SDRAD_ERROR_DOMAIN_NO_MAP);
        return (void *)SDRAD_ERROR_NO_DOMAIN;
    }

    SDRAD_MUTEX_LOCK();
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi]; 
    if (sdi_ptr -> sdi_init_heap_flag == false){
        parent_domain_sti = sgm_ptr -> sgm_sdi_to_tdi[sdi]; 
        if(parent_domain_sti != thread_index){
            stm_parent_ptr = sgm_ptr -> stm_ptr[parent_domain_sti]; // get parent thread 
            shm_parent_ptr = (sdrad_d_info_t *)&stm_parent_ptr -> sdrad_d_info[sdi];  // parent domain info
            if(shm_parent_ptr -> sdi_init_heap_flag == false){
                sdrad_heap_domain_init(shm_parent_ptr, sgm_ptr-> pdi[sdi]);
            }
            memcpy(sdi_ptr, shm_parent_ptr, sizeof(sdrad_d_info_t));
            sdi_ptr -> sdi_init_heap_flag = true;
        }else{
            sdrad_heap_domain_init(sdi_ptr, sgm_ptr-> pdi[sdi]);
        }

    }

    if(sdi_ptr -> sdi_tlsf == NULL){
        assert(sdi_ptr -> sdi_tlsf != NULL); 
    }

    ptr = tlsf_malloc(sdi_ptr -> sdi_tlsf, size);
    SDRAD_MUTEX_UNLOCK();
    active_domain = stm_ptr -> active_domain;
    // return to old pkru value
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return ptr;   
}

void *sdrad_calloc(udi_t udi, size_t nelem, size_t elsize)
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_thread_metadata_t      *stm_ptr, *stm_parent_ptr; 
    sdrad_d_info_t               *sdi_ptr,*shm_parent_ptr;;
    int32_t                      sdi;
    sdrad_domain_t              active_domain; 
    udi_t                       parent_domain_sti;

    sdrad_store_pkru_config(PKRU_ALL_UNSET);  //can access all domain
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    //Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head);
    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP){
        assert(sdi != SDRAD_ERROR_DOMAIN_NO_MAP);
        return (void *)SDRAD_ERROR_NO_DOMAIN;
    } 

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi]; 
    SDRAD_MUTEX_LOCK();
    if (sdi_ptr -> sdi_init_heap_flag == false){
        parent_domain_sti = sgm_ptr -> sgm_sdi_to_tdi[sdi]; 
        if(parent_domain_sti != thread_index){
            stm_parent_ptr = sgm_ptr -> stm_ptr[parent_domain_sti]; // get parent thread 
            shm_parent_ptr = (sdrad_d_info_t *)&stm_parent_ptr -> sdrad_d_info[sdi];  // parent domain info
            memcpy(sdi_ptr, shm_parent_ptr, sizeof(sdrad_d_info_t));
            sdi_ptr -> sdi_init_heap_flag = true;
        }else{
            sdrad_heap_domain_init(sdi_ptr, sgm_ptr-> pdi[sdi]);
        }
    }

    register void *ptr;  
    if (nelem == 0 || elsize == 0)
        nelem = elsize = 1;

    ptr = tlsf_malloc(sdi_ptr -> sdi_tlsf, nelem * elsize); //sdradmalloc
    SDRAD_MUTEX_UNLOCK();
    if (ptr) bzero (ptr, nelem * elsize);
    active_domain = stm_ptr -> active_domain;
    // return to old pkru value
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return ptr; 
}





void sdrad_heap_domain_init(sdrad_d_info_t *sdi_ptr, int32_t pkey)
{
    int32_t status; 
    sdrad_domain_size_t             heap_size; 
    sdrad_da_t                      address; 

    sdi_ptr -> sdi_size_of_heap = SDRAD_DEFAULT_HEAP_SIZE;
    sdi_ptr -> sdi_address_of_heap = (sdrad_da_t)mmap(NULL,  sdi_ptr -> sdi_size_of_heap,
                                                    PROT_READ | PROT_WRITE,
                                                    MAP_PRIVATE |
                                                    MAP_ANONYMOUS,
                                                    -1,
                                                    0);
    
    //mprotect((void *)sdi_ptr -> sdi_address_of_heap, 4096, PROT_NONE);
    //mprotect((void *)(sdi_ptr -> sdi_address_of_heap+sdi_ptr -> sdi_size_of_heap-4096), 4096, PROT_NONE);
    //sdi_ptr -> sdi_address_of_heap = sdi_ptr -> sdi_address_of_heap + 4096;
    //sdi_ptr -> sdi_size_of_heap = sdi_ptr -> sdi_size_of_heap - 2*4096;
    
    status = pkey_mprotect((void *)sdi_ptr -> sdi_address_of_heap, 
                            sdi_ptr -> sdi_size_of_heap,
                            PROT_READ | PROT_WRITE, 
                            pkey);
                                
    
    assert(status != -1);
    if(sdi_ptr -> sdi_size_of_heap <= SDRAD_TLSF_MAX_HEAP_SIZE){
        sdi_ptr -> sdi_tlsf = tlsf_create_with_pool((void *)sdi_ptr -> sdi_address_of_heap, 
                                                sdi_ptr -> sdi_size_of_heap);
    }else{
        sdi_ptr -> sdi_tlsf = tlsf_create_with_pool((void *)sdi_ptr -> sdi_address_of_heap, 
                                                SDRAD_TLSF_MAX_HEAP_SIZE);
        heap_size = sdi_ptr -> sdi_size_of_heap - SDRAD_TLSF_MAX_HEAP_SIZE;
        address =  sdi_ptr -> sdi_address_of_heap + SDRAD_TLSF_MAX_HEAP_SIZE;
        while (heap_size  > SDRAD_TLSF_MAX_HEAP_SIZE)
        {
            tlsf_add_pool(sdi_ptr -> sdi_tlsf, 
                        (void *)address, 
                        SDRAD_TLSF_MAX_HEAP_SIZE);
            heap_size = heap_size - SDRAD_TLSF_MAX_HEAP_SIZE;
            address = address + SDRAD_TLSF_MAX_HEAP_SIZE;
        } 
        tlsf_add_pool(sdi_ptr -> sdi_tlsf, 
                        (void *)address, 
                        heap_size);

    }
    sdi_ptr -> sdi_init_heap_flag = true; 

    SDRAD_DEBUG_PRINT("[SDRAD] Heap Domain  %d Initialized %p \n", 
                        stm_ptr->active_domain, 
                        sdi_ptr->sdi_address_of_heap); 

}

/**
 * @brief get memory area from Monitor Data Domain area
 * 
 * @param size 
 * @return void* 
 */
void *sdrad_malloc_mdd(size_t size)
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_monitor_domain_t       *smd_ptr;
    void   *ptr; 

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    smd_ptr = (sdrad_monitor_domain_t *)&sgm_ptr -> sdrad_monitor_domain;
    ptr = tlsf_malloc(smd_ptr -> smd_tlsf, size);
    return ptr; 
}

