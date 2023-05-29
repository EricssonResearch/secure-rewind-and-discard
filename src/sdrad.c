/**
 * @file sdrad.c
 * @author Merve Gulmez 
 * @brief 
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
#include <sys/types.h>
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
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <alloca.h>
#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include "/usr/include/event.h"
#include <sys/resource.h> 
#include <linux/unistd.h>
#include <asm/ldt.h>
#include "sdrad.h"
#include "sdrad_pkey.h"
#include "sdrad_api.h"
#include "tlsf.h" 
#include "sdrad_heap_mng.h"
#include "sdrad_thread.h"
#include "sdrad_signal.h"
#include "sdrad_cache.h"



/* SDRoB Monitor Domain Global Info */
sdrad_global_manager_t  sdrad_global_manager __attribute__((aligned(0x1000)));

pthread_mutex_t  malloc_mutex = PTHREAD_MUTEX_INITIALIZER;  




/*I cannot put here SDRAD_MUTEX_LOCK and unlock..
* It affects the wrap_pthread_create function
* the thread cannot be initialized in that case */

int32_t sdrad_find_free_domain(sdrad_global_manager_t *sgm_ptr)
{
    int32_t i; 
    for (i = 0 ; i < TOTAL_DOMAIN; i++ ){
        if((sgm_ptr->status[i]) == DOMAIN_NONOCCUPIED){
            sgm_ptr->status[i] = DOMAIN_OCCUPIED;
            return i; 
        }
    }
    return -1; 
}


sdrad_domain_size_t sdrad_get_default_stack_size()
{
    struct rlimit rlim;
    sdrad_domain_size_t stack_size;

    if(getrlimit(RLIMIT_STACK, &rlim) != 0){
        SDRAD_DEBUG_PRINT("getrlimit failed");
        stack_size = 1024*PAGE_SIZE;
    }else{
        stack_size = rlim.rlim_cur;
    }
    return stack_size;
}

void sdrad_get_config(sdrad_d_info_t *sdi_ptr)
{
    char *pTmp;

    pTmp = getenv( "SDRAD_STACK_SIZE");

    if( pTmp != NULL){
        sdi_ptr -> sdi_size_of_stack = atoi(pTmp); 
    }else{
        sdi_ptr -> sdi_size_of_stack = SDRAD_DEFAULT_STACK_SIZE;
    }

    pTmp = getenv( "SDRAD_HEAP_SIZE");

    if(pTmp != NULL){
        sdi_ptr -> sdi_size_of_heap = atoi(pTmp); 
    }else{
        sdi_ptr -> sdi_size_of_heap = SDRAD_DEFAULT_HEAP_SIZE; 
    }
}


sdrad_domain_t sdrad_domain_init()
{
    sdrad_d_info_t               *sdi_ptr; 
    sdrad_thread_metadata_t      *stm_ptr; 
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_domain_t               free_domain; 

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    //Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    assert(thread_index != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    free_domain = sdrad_find_free_domain(sgm_ptr);
    assert(free_domain != 1);  
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[free_domain]; 
    sdrad_get_config(sdi_ptr);
    return free_domain;
} 

/**
 * @brief just for getting a domain from sdrad manager, and get configuration from the enviromental
 *        varible
 * 
 * @return sdrad_domain_t 
 */
// no mutex here, unfortunately, pthread create destroy 
sdrad_domain_t sdrad_request_sdi()
{
    sdrad_thread_metadata_t      *stm_ptr;
    sdrad_d_info_t               *sdi_ptr; 
    sdrad_global_manager_t       *sgm_ptr;
    int32_t                     free_domain;

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    //Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    assert(thread_index != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    free_domain = sdrad_find_free_domain(sgm_ptr); 
    assert(free_domain != -1); 
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[free_domain]; 
    sdrad_get_config(sdi_ptr); 
    return free_domain;
} 


sdrad_domain_t sdrad_create_monitor_domain()
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_monitor_domain_t       *smd_ptr;
    char                         *pTmp;


    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    smd_ptr = (sdrad_monitor_domain_t  *)&sgm_ptr -> sdrad_monitor_domain; 
    pTmp = getenv( "SDRAD_HEAP_SIZE");
    if(pTmp != NULL){
        smd_ptr -> smd_size_of_data_domain = atoi(pTmp); 
    }else{
        smd_ptr -> smd_size_of_data_domain = 0x100000000;
    }

    smd_ptr -> smd_address_data_domain = (sdrad_da_t)mmap(NULL, 
                                            smd_ptr -> smd_size_of_data_domain,
                                            PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE |
                                            MAP_ANONYMOUS,
                                            -1, 0);

    sgm_ptr -> pdi[MONITOR_DATA] = sdrad_allocate_pkey(smd_ptr -> smd_address_data_domain,
                                                       smd_ptr -> smd_size_of_data_domain); 
    


    smd_ptr -> smd_tlsf = tlsf_create_with_pool((void *)smd_ptr -> smd_address_data_domain, 
                                                smd_ptr -> smd_size_of_data_domain);

    return MONITOR_DATA;
} 





sdrad_da_t do_page_aligned(int64_t rsp) 
{
    return  rsp & ~(PAGE_SIZE - 1);
}

sdrad_da_t do_up_page_aligned(int64_t rsp) 
{
    return  (rsp & ~(PAGE_SIZE - 1)) + 4096;
}



size_t  get_default_stack_size() 
{
    struct rlimit rlim;
    size_t stack_size = 0;
    if(getrlimit(RLIMIT_STACK, &rlim) == 0)
        stack_size = rlim.rlim_cur;
    return stack_size;
}

void *(*real_mmap)(void *, size_t, int, int, int, off_t) = NULL;
int   (*real_munmap)(void *addr, size_t length) = NULL;


/**
 * @brief it runs before main() to assign all application stack and heap area
 * global area to Root Domain
 *   
 */

int sdrad_constructor_flag = false; 
__attribute__((constructor))
void sdrad_constructor()
{
    sdrad_d_info_t               *sdi_ptr;
    sdrad_thread_metadata_t      *stm_ptr; 
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_monitor_domain_t       *smd_ptr;
    sdrad_da_t                   bss_end_pa; 
    int32_t status; 
    int32_t i; 

    sdrad_constructor_flag = true; 
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;  
    smd_ptr = (sdrad_monitor_domain_t *)&sgm_ptr -> sdrad_monitor_domain; 
 
    SDRAD_DEBUG_PRINT("[SDRAD] Initialization Started \n");

    for (i = DOMAIN_2; i < TOTAL_DOMAIN ; i++ ){
        sgm_ptr -> status[i] = DOMAIN_NONOCCUPIED; 
    }
    /* Create  Monitor Data Domain */
    smd_ptr -> smd_monitor_sdi = sdrad_create_monitor_domain();
	/* Mapping TID (Thread ID) to STI (SDRoB Thread Index). */
    sgm_ptr -> sgm_thread_id[ROOT_DOMAIN] = pthread_self(); 
    sgm_ptr -> sgm_sdi_to_tdi[ROOT_DOMAIN] = ROOT_DOMAIN;

    sgm_ptr -> sgm_total_thread = ROOT_DOMAIN;
    /* Create Thread Meta Data from Root Domain */
    sgm_ptr -> stm_ptr[sgm_ptr -> sgm_total_thread] = sdrad_malloc_mdd(sizeof(sdrad_thread_metadata_t)); 
    stm_ptr = sgm_ptr -> stm_ptr[sgm_ptr -> sgm_total_thread]; 
    sgm_ptr -> stm_root_ptr = stm_ptr;

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[ROOT_DOMAIN];  /// thread local data
    sdi_ptr -> sdi_parent_domain = ROOT_DOMAIN;
    sgm_ptr -> sgm_total_thread++; 
    sgm_ptr -> status[ROOT_DOMAIN] = DOMAIN_OCCUPIED;
    sgm_ptr -> status[MONITOR_DATA] = DOMAIN_OCCUPIED;

    /* root domain is an initial security domain */
    stm_ptr -> active_domain = ROOT_DOMAIN;

    /* Check the environment variables */
    sdrad_get_config(sdi_ptr); 

    /* SDRoB mutex is initialized */
    sdrad_mutex_constructor(sgm_ptr); 
    /*	Setup the signal handler */
    sdrad_setup_signal_handler();

    sgm_ptr -> pdi[ROOT_DOMAIN] = pkey_alloc(0,0); 

    assert(sgm_ptr -> pdi[ROOT_DOMAIN] != -1); 
    /*Do page aligned bss end */ 
    bss_end_pa = do_page_aligned((int64_t)&end) +  4096 ;

    /*Data segment + BSS segment protection */ 
    status = pkey_mprotect(&__data_start, 
                            bss_end_pa - (int64_t)&__data_start, 
                            PROT_READ | PROT_WRITE, 
                            sgm_ptr -> pdi[ROOT_DOMAIN]);  
    assert(status != -1); 

    /* Associate global monitor data with PKRU */

    status = pkey_mprotect(&sdrad_global_manager,
                           sizeof(sdrad_global_manager_t), 
                           PROT_READ | PROT_WRITE,
                           sgm_ptr -> pdi[smd_ptr -> smd_monitor_sdi]); 
    assert(status != -1); 


    sdi_ptr -> sdi_size_of_stack = get_default_stack_size();
    sdi_ptr -> sdi_address_stack = mmap(NULL,                                  
                sdi_ptr -> sdi_size_of_stack,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_STACK |MAP_GROWSDOWN |
                MAP_ANONYMOUS,
                -1,
                0);

    status = pkey_mprotect((void *)sdi_ptr -> sdi_address_stack, 
                            sdi_ptr -> sdi_size_of_stack, 
                            PROT_READ | PROT_WRITE, 
                            sgm_ptr -> pdi[ROOT_DOMAIN]);  

    assert(status != -1); 

    stm_ptr -> pkru_config[ROOT_DOMAIN] = sdrad_pkru_default_config(sgm_ptr -> pdi[ROOT_DOMAIN]);
    sdrad_store_pkru_config(stm_ptr -> pkru_config[ROOT_DOMAIN]); 
}

struct timespec start, stop;
double accum;
#define BILLION 1E9

/**
 * @brief signal handler + stack_check_fail triggers
 * 
 * @return int32_t 
 */
int32_t sdrad_abnormal_domain_exit()
{
    sdrad_thread_metadata_t  *stm_ptr; 
    sdrad_global_manager_t   *sgm_ptr;
    sdrad_d_info_t    *sdi_ptr; 
    sdrad_domain_t  active_domain; 

    sdrad_store_pkru_config(PKRU_ALL_UNSET); 
 
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    //Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    assert(thread_index != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    active_domain = stm_ptr -> active_domain;
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[active_domain]; 
    sdrad_setup_signal_handler();
    // no return for PDI root domain 
    if(sgm_ptr -> pdi[active_domain] == sgm_ptr -> pdi[ROOT_DOMAIN]){
        abort();  
    }else{
        longjmp(sdi_ptr -> sdi_buffer, 1);
    }
}

int sdrad_search_udi_control(udi_t udi, udi_control_t *udi_ptr)
{
    SDRAD_MUTEX_LOCK(); 
    // Iterate till last element until key is not found
    while (udi_ptr != NULL && udi_ptr->udi != udi)
    {
        udi_ptr = udi_ptr -> next;
    }
    SDRAD_MUTEX_UNLOCK(); 
    return (udi_ptr != NULL) ? udi_ptr -> sdi: SDRAD_ERROR_DOMAIN_NO_MAP;
}


sdrad_da_t sdrad_allocate_stack(sdrad_domain_size_t stack_size)
{
    void *sdi_address_stack; 
    sdi_address_stack = mmap(NULL,                                  
                    stack_size,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_STACK |MAP_GROWSDOWN |
                    MAP_ANONYMOUS,
                    -1,
                    0);

    assert(sdi_address_stack != MAP_FAILED);

    return (sdrad_da_t)sdi_address_stack;
}




void sdrad_destroy_domain(sdrad_global_manager_t  *sgm_ptr,
                         sdrad_d_info_t           *sdi_ptr,
                         int32_t active_domain)
{

    sdrad_cache_domain_info_t    *scdi_ptr;
    volatile void                *ptr; 
    volatile size_t              domain_size; 

    scdi_ptr = sdrad_insert_cache_domain(sgm_ptr);
    scdi_ptr -> scdi_address_stack = sdi_ptr -> sdi_address_stack; 
    scdi_ptr -> scdi_size_of_stack = sdi_ptr -> sdi_size_of_stack;
    scdi_ptr -> scdi_pkey = sgm_ptr -> pdi[active_domain]; 
    ptr = (void *)scdi_ptr -> scdi_address_stack; 
    domain_size = scdi_ptr -> scdi_size_of_stack; 
#ifdef ZERO_DOMAIN
    explicit_bzero(ptr, domain_size);
#endif
    if(sdi_ptr -> sdi_init_heap_flag == true){
        scdi_ptr -> scdi_init_heap_domain_flag = true; 
        scdi_ptr -> scdi_address_heap = sdi_ptr -> sdi_address_of_heap; 
        scdi_ptr -> scdi_size_of_heap = sdi_ptr -> sdi_size_of_heap;
        scdi_ptr -> scdi_tlsf = NULL;
        ptr = (void *)scdi_ptr -> scdi_address_heap; 
        domain_size = scdi_ptr -> scdi_size_of_heap;
#ifdef ZERO_DOMAIN
        explicit_bzero(ptr, domain_size);
#endif
    }
    sdrad_delete_control_structure(sgm_ptr, active_domain); 
}

/*
int32_t sdrad_search_udi_control(udi_t udi, sdrad_global_manager_t *sgm_ptr)
{
    for(int32_t k = 0; k < TOTAL_DOMAIN; k++)
    {
        if(sgm_ptr -> sgm_udi_to_sdi[k] == udi)
            return k;
    }
    return SDRAD_ERROR_DOMAIN_NO_MAP;
}
*/
int32_t sdrad_map_udi_to_sdi(udi_t udi, sdrad_global_manager_t *sgm_ptr)
{
    int32_t     sdi; 

    sdi = sdrad_find_free_domain(sgm_ptr); 
    assert(sdi != -1);
    sgm_ptr -> sgm_udi_to_sdi[sdi] = udi; 
    return sdi; 
}


int32_t sdrad_check_init_config(uint64_t domain_feature)
{
    int32_t flags = domain_feature & (SDRAD_DATA_DOMAIN|SDRAD_EXECUTION_DOMAIN);

    if (flags == (SDRAD_DATA_DOMAIN|SDRAD_EXECUTION_DOMAIN)){ /* can't be both */
		return SDRAD_ERROR_WRONG_CONFIG;
    }
    
    flags = domain_feature & (SDRAD_ISOLATED_DOMAIN|SDRAD_NONISOLATED_DOMAIN);

    if (flags == (SDRAD_ISOLATED_DOMAIN|SDRAD_NONISOLATED_DOMAIN)){ /* can't be both */
		return SDRAD_ERROR_WRONG_CONFIG;  
    }
    flags = domain_feature & (SDRAD_RETURN_TO_PARENT|SDRAD_RETURN_TO_CURRENT);

    if (flags == (SDRAD_RETURN_TO_PARENT|SDRAD_RETURN_TO_CURRENT)){/* can't be both */
		return SDRAD_ERROR_WRONG_CONFIG; 
    }

    return 1; 
}


void sdrad_execution_domain_init(sdrad_global_manager_t *sgm_ptr,
                                sdrad_d_info_t *sdi_ptr,     
                                int32_t pkey)
{
    sdrad_cache_domain_info_t *scdi_ptr;
    int32_t     status; 

    if(sgm_ptr -> sgm_head != NULL){
        scdi_ptr = sgm_ptr -> sgm_head;
        sdi_ptr -> sdi_address_stack = scdi_ptr -> scdi_address_stack; 
        sdi_ptr -> sdi_size_of_stack = scdi_ptr -> scdi_size_of_stack;

        if(scdi_ptr -> scdi_init_heap_domain_flag == true){
            sdi_ptr -> sdi_address_of_heap = scdi_ptr -> scdi_address_heap; 
            sdi_ptr -> sdi_size_of_heap = scdi_ptr -> scdi_size_of_heap;
            sdi_ptr -> sdi_tlsf = tlsf_create_with_pool((void *)sdi_ptr -> sdi_address_of_heap,
                                                    sdi_ptr -> sdi_size_of_heap);
        }else{
            sdi_ptr -> sdi_init_heap_flag = false;
        }
        sdrad_delete_cache_domain(sgm_ptr);
        return;
    }
    sdrad_get_config(sdi_ptr); 

    sdi_ptr -> sdi_address_stack = sdrad_allocate_stack(sdi_ptr -> sdi_size_of_stack);

#ifdef PKEY_ENABLE
    status = pkey_mprotect((void *)sdi_ptr -> sdi_address_stack,
                                sdi_ptr -> sdi_size_of_stack, 
                                PROT_READ | PROT_WRITE, 
                                pkey);  

    assert(status  != -1);
#endif
}

void sdrad_data_domain_init(sdrad_global_manager_t *sgm_ptr,
                            sdrad_d_info_t *sdi_ptr, int32_t pkey)
{
    struct                          sdrad_cache_domain_info_s   *scdi_ptr;
    char                            *pTmp;
    int32_t                             status; 
    sdrad_domain_size_t             heap_size; 
    sdrad_da_t                      address; 

    if(sgm_ptr -> sgm_head != NULL ) {
        if(sgm_ptr -> sgm_head -> scdi_init_heap_domain_flag == true){
            scdi_ptr =  sgm_ptr -> sgm_head;
            sdi_ptr -> sdi_address_of_heap = scdi_ptr -> scdi_address_heap; 
            sdi_ptr -> sdi_size_of_heap = scdi_ptr -> scdi_size_of_heap;
            sdi_ptr -> sdi_tlsf = tlsf_create_with_pool((void *)sdi_ptr -> sdi_address_of_heap,
                                                    sdi_ptr -> sdi_size_of_heap);
            sdi_ptr -> sdi_init_heap_flag = true; 
            sdrad_delete_cache_domain(sgm_ptr);
            return;
        }
    }

    if (sdi_ptr -> sdi_init_heap_flag == false) {
        pTmp = getenv( "SDRAD_DATA_DOMAIN_SIZE");
        if(pTmp != NULL){
            sdi_ptr -> sdi_size_of_heap = atoi(pTmp); 
        }else{
            sdi_ptr -> sdi_size_of_heap = SDRAD_DEFAULT_DATA_DOMAIN_SIZE;
        }

        sdi_ptr -> sdi_address_of_heap = (sdrad_da_t)mmap(NULL, 
                                                sdi_ptr -> sdi_size_of_heap,
                                                PROT_READ | PROT_WRITE,
                                                MAP_PRIVATE |
                                                MAP_ANONYMOUS,
                                                -1, 0);

        status = pkey_mprotect((void *)sdi_ptr -> sdi_address_of_heap, 
                               sdi_ptr -> sdi_size_of_heap, 
                               PROT_READ | PROT_WRITE, 
                               pkey);  
        
        assert(status  != -1);
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
        }
        sdi_ptr -> sdi_init_heap_flag = true; 
    }
} 



tlsf_t *sdrad_search_tlsf_control(void *ptr, sdrad_global_manager_t *sgm_ptr)
{
    udi_control_t       *udi_ptr;
    sdrad_d_info_t      *sdi_ptr;
    sdrad_thread_metadata_t           *stm_ptr; 
        
    //Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];


    udi_ptr = sgm_ptr -> udi_control_head;

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[ROOT_DOMAIN];
    if((ptr > sdi_ptr -> sdi_tlsf) && ptr < (sdi_ptr -> sdi_tlsf + sdi_ptr -> sdi_size_of_heap))
        return sdi_ptr -> sdi_tlsf; 

    SDRAD_MUTEX_LOCK(); 
    // Iterate till last element until key is not found
    while (udi_ptr != NULL){
        sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[udi_ptr -> sdi];
        if((ptr > sdi_ptr -> sdi_tlsf) && ptr < (sdi_ptr -> sdi_tlsf + sdi_ptr -> sdi_size_of_heap))
            return sdi_ptr -> sdi_tlsf; 
        udi_ptr = udi_ptr -> next;
    }
    SDRAD_MUTEX_UNLOCK(); 
    return NULL;
}


udi_control_t *sdrad_insert_control_structure(sdrad_global_manager_t *sgm_ptr)
{
    udi_control_t     *udi_ptr; 

    SDRAD_MUTEX_LOCK(); 
    udi_ptr = sdrad_malloc_mdd(sizeof(udi_control_t));
    assert(udi_ptr != NULL);
    // point it to old first node
    udi_ptr -> next = sgm_ptr -> udi_control_head;
    //point first to new first node
    sgm_ptr -> udi_control_head = udi_ptr;
    SDRAD_MUTEX_UNLOCK(); 

    return udi_ptr;
}

//delete a link with given key
void sdrad_delete_control_structure(sdrad_global_manager_t *sgm_ptr, 
                                             int32_t sdi) 
{
    //start from the first link
    udi_control_t   *temp; 
    udi_control_t   *head; 
    udi_control_t  *current; 

    head = sgm_ptr -> udi_control_head; 
    current = sgm_ptr -> udi_control_head;

    //if list is empty
    if(head == NULL) {
        return NULL;
    }
     
    if(sgm_ptr -> udi_control_head -> sdi == sdi){
        temp = sgm_ptr -> udi_control_head;    //backup to free the memory
        sgm_ptr -> udi_control_head = sgm_ptr -> udi_control_head ->next;
    //    free(temp);
    }
    else{
        while(current->next != NULL){
            if(current->next->sdi == sdi){
                temp = current->next;
                //node will be disconnected from the linked list.
                current->next = current->next->next;
                free(temp);
                break;
            }else{
                current = current->next;
            }
        }
    }

}

// void  *sdrad_search_thread_structure()
// {
//     sdrad_thread_metadata_t           *stm_ptr; 
//     sdrad_global_manager_t            *sgm_ptr;

//     sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;

//     stm_ptr = sgm_ptr -> stm_head_ptr;
//     sti_t tid = pthread_self();
//     while (stm_ptr != NULL)
//     {
//         if(stm_ptr->thread_id == tid)
//             return stm_ptr;
//         stm_ptr = stm_ptr-> next;
//     }
// }

// void *sdrad_insert_thread_structure(sdrad_global_manager_t *sgm_ptr)
// { 
//     sdrad_thread_metadata_t           *stm_ptr; 


//     stm_ptr = sdrad_malloc_mdd(sizeof(sdrad_thread_metadata_t));
//     // point it to old first node
//     stm_ptr -> thread_id = pthread_self();
//     stm_ptr -> next = sgm_ptr -> stm_head_ptr;
//     //point first to new first node
//     sgm_ptr -> stm_head_ptr = stm_ptr;
//     return stm_ptr;
// }


/* Sdrob_init calls that functions in case of abnormal domain exit */
udi_t  sdrad_init_destroy(udi_t udi)
{
    sdrad_d_info_t             *sdi_ptr, *sdi_parent_ptr, *sdi_grand_parent_ptr; 
    sdrad_thread_metadata_t    *stm_ptr; 
    sdrad_global_manager_t     *sgm_ptr;
    int32_t                    active_domain, parent_domain, grant_parent_domain;
    sdrad_init_stack_t         *ss_ptr;

    sdrad_store_pkru_config(PKRU_ALL_UNSET); 
    //Get STI from TDI 
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    int32_t thread_index = sdrad_get_sti(sgm_ptr); 

    int32_t sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head);

    assert(thread_index != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    active_domain = stm_ptr -> active_domain; //openssl udi
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[active_domain]; 

    parent_domain = sdi_ptr -> sdi_parent_domain;  //nested domain

    if(sdi_ptr -> sdi_options & SDRAD_RETURN_TO_PARENT){
        sdi_grand_parent_ptr =  (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[parent_domain]; //nested_domain_ptr
        grant_parent_domain = sdi_grand_parent_ptr -> sdi_parent_domain; // root domain
        stm_ptr -> active_domain = grant_parent_domain; 
        sdrad_destroy_domain(sgm_ptr, sdi_ptr, parent_domain);  //nested domain
    }else{
        stm_ptr -> active_domain = parent_domain; 
    }

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[active_domain]; 
    ss_ptr = (sdrad_init_stack_t *)sdi_ptr -> sdi_base_address;

    sdrad_destroy_domain(sgm_ptr, sdi_ptr, active_domain); //openssl destroy

    sgm_ptr -> status[active_domain] = DOMAIN_NONOCCUPIED;  

    stm_ptr -> active_domain = sdi_ptr -> sdi_parent_domain; 
    sdi_parent_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[stm_ptr -> active_domain]; 
    tlsf_structure = sdi_parent_ptr -> sdi_tlsf; 
    ss_ptr ->  return_address = sdi_ptr -> sdi_return_address; 
    ss_ptr ->  udi = stm_ptr-> pkru_config[stm_ptr -> active_domain]; //it should be pkru

    // delete udi to sdi map 
    sgm_ptr -> sgm_udi_to_sdi[sdi] = 0; 

    // delete pdi to sdi map
    sgm_ptr -> pdi[sdi] = 0;

    return udi;
}


void __sdrad_exit_root(void *base_address)
{
    sdrad_thread_metadata_t      *stm_ptr; 
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_d_info_t               *sdi_ptr;
    sdrad_d_info_t               *sdi_parent_ptr;
    int32_t                      sdi, sti;
    sdrad_ee_stack_t  *ss_ptr; 


    sdrad_store_pkru_config(PKRU_ALL_UNSET);
    ss_ptr = (sdrad_ee_stack_t *)base_address;
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    // Get STI from TDI 
    sti = sdrad_get_sti(sgm_ptr); 
    assert(sti != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[sti];

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[ROOT_DOMAIN];
    ss_ptr -> stack_info = sdi_ptr -> sdi_root_rsp; 
    ss_ptr -> pkru_info = stm_ptr -> pkru_config[ROOT_DOMAIN];
}



void __sdrad_enter_root(void *base_address)
{
    sdrad_d_info_t              *sdi_ptr;
    sdrad_thread_metadata_t     *stm_ptr; 
    sdrad_global_manager_t      *sgm_ptr;
    int32_t                     sdi, sti;
    sdrad_ee_stack_t            *ss_ptr; 

    sdrad_store_pkru_config(PKRU_ALL_UNSET);
    ss_ptr = (sdrad_ee_stack_t *)base_address;

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;

    // Get STI from TDI 
    sti = sdrad_get_sti(sgm_ptr); 
    assert(sti != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[sti];

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[ROOT_DOMAIN]; 
    /* parent domain stack pointer doesn't cover current return address */
    sdi_ptr -> sdi_root_rsp = (sdrad_da_t)ss_ptr -> stack_info + 16;    
    ss_ptr -> stack_info = (sdi_ptr -> sdi_address_stack + sdi_ptr -> sdi_size_of_stack); 
    ss_ptr -> pkru_info = stm_ptr -> pkru_config[ROOT_DOMAIN];
}


static int (*real_main) (int, char **, char **);
static int fake_main(int argc, char **argv, char **envp)
{	
    sdrad_enter_root(); 
	/* Finally call the real main function */
	int ret = real_main(argc, argv, envp);
    sdrad_exit_root();
    return ret; 
}


int __libc_start_main(int (*main) (int, char **, char **),
		      int argc, char **ubp_av, void (*init) (void),
		      void (*fini) (void), void (*rtld_fini) (void),
		      void (*stack_end))
{
	void *libc_handle, *sym;
	
	union {
		int (*fn) (int (*main) (int, char **, char **), int argc,
			   char **ubp_av, void (*init) (void),
			   void (*fini) (void), void (*rtld_fini) (void),
			   void (*stack_end));
		void *sym;
	} real_libc_start_main;

	libc_handle = dlopen("libc.so.6", RTLD_NOLOAD | RTLD_NOW);

	if (!libc_handle) {
		fprintf(stderr, "can't open handle to libc.so.6: %s\n",
			dlerror());
		_exit(EXIT_FAILURE);
	}
	
	sym = dlsym(libc_handle, "__libc_start_main");
	if (!sym) {
		fprintf(stderr, "can't find __libc_start_main():%s\n",
			dlerror());
		_exit(EXIT_FAILURE);
	}

	real_libc_start_main.sym = sym;
	real_main = main;
	
	if(dlclose(libc_handle)) {
		fprintf(stderr, "can't close handle to libc.so.6: %s\n",
			dlerror());
		_exit(EXIT_FAILURE);
	}

	/* Note that we swap fake_main in for main - fake_main should call
	 * real_main after its setup is done. */
	return real_libc_start_main.fn(fake_main, argc, ubp_av, init, fini,
				       rtld_fini, stack_end);
}
