/**
 * @file sdrad.h
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
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "tlsf.h" 


#define PAGE_SIZE 4096

#define NDEBUG 


#define DOMAIN_INIT_REQ                 39


extern struct timespec start, stop;

extern int8_t __data_start;
extern int8_t __bss_start;
extern int8_t __executable_start; 

#ifdef SDRAD_MULTITHREAD
extern __thread tlsf_t tlsf_structure; 
#else
extern tlsf_t tlsf_structure;
#endif


extern pthread_mutex_t  malloc_mutex;  
extern int sdrad_constructor_flag;  


/* https://linux.die.net/man/3/etext*/
extern int32_t etext; 
extern int32_t edata;
extern int32_t end;


#define SDRAD_DEFAULT_STACK_SIZE              0x800000
#define SDRAD_DEFAULT_HEAP_SIZE               0x100000000  
#define SDRAD_TLSF_MAX_HEAP_SIZE              0x100000000
#define SDRAD_DEFAULT_DATA_DOMAIN_SIZE        0x100000000



// SDRAD_DEBUG_PRINT should be enabled with compiler flag! 
#ifdef SDRAD_DEBUG_PRINT
#define SDRAD_DEBUG_PRINT(fmt, ...) \
            do {                    \
            fprintf(stderr, fmt, ##__VA_ARGS__); \
            } while (0)
#else
#define SDRAD_DEBUG_PRINT(fmt, ...) \
            do {                    \
            } while (0)        
#endif


#define SDRADEXIT(msg) \
    do {perror(msg); exit(EXIT_FAILURE); \
        } while (0)



typedef uint64_t      sdrad_da_t; 
typedef int32_t       sdrad_domain_t; 
typedef uint64_t      sdrad_domain_size_t;
typedef uint64_t      udi_t;
typedef uint64_t      sti_t;
typedef int64_t       sdrad_pkru_t;



#define NUM_OF_THREADS 200                                                   



typedef struct{
    jmp_buf buffer;
    int64_t domain_feature;
    udi_t udi;
    int64_t return_address;
}__attribute__((packed))sdrad_init_stack_t;


/*sdrad_enter and exit assembly code */
typedef struct{
    int64_t          stack_info;
    sdrad_pkru_t     pkru_info;
}__attribute__((packed)) sdrad_ee_stack_t;



enum
{
    MONITOR_DATA,
    ROOT_DOMAIN,
    DOMAIN_2,
    TOTAL_DOMAIN = 30
}; 

//Usage status
enum {
    DOMAIN_OCCUPIED = 55,
    DOMAIN_NONOCCUPIED = 56 
};



typedef struct {
    uint32_t            status;                  // DOMAIN_OCCUPIED
    sdrad_domain_size_t sdi_size_of_stack;
    sdrad_domain_t      sdi_parent_domain;
    sdrad_da_t          sdi_last_rsp; 
    sdrad_da_t          sdi_root_rsp;   
    sdrad_da_t          sdi_address_stack; 
    sdrad_da_t          sdi_address_offset_stack;
    jmp_buf             sdi_buffer;
    int64_t             sdi_return_address;
    sdrad_da_t          sdi_base_address;
    bool                sdi_saved_ex; 
    tlsf_t              sdi_tlsf;
    int64_t             sdi_options;       
    sdrad_domain_size_t sdi_size_of_heap; 
    bool                sdi_init_heap_flag; 
    sdrad_da_t          sdi_address_of_heap;
}sdrad_d_info_t; 


struct sdrad_pthread_constructor_s{
    void    *real_start_routine;
    void    *arg;
};


typedef struct {
    tlsf_t                  smd_tlsf;
    sdrad_domain_size_t     smd_size_of_data_domain; 
    sdrad_da_t              smd_address_data_domain;
    sdrad_domain_t          smd_monitor_sdi; 
}sdrad_monitor_domain_t;

typedef struct sdrad_cache_domain_info_s{
    tlsf_t                              scdi_tlsf;
    sdrad_domain_size_t                 scdi_size_of_stack; 
    sdrad_domain_size_t                 scdi_size_of_heap; 
    sdrad_da_t                          scdi_address_stack; 
    sdrad_da_t                          scdi_address_heap; 
    bool                                scdi_init_heap_domain_flag;
    sdrad_pkru_t                        scdi_pkey;
    struct sdrad_cache_domain_info_s   *scdi_next;
}sdrad_cache_domain_info_t;


typedef struct udi_control_s{
    udi_t                       udi; 
    int32_t                     sdi; 
    sdrad_d_info_t              *sdi_ptr; 
    struct udi_control_s        *next;
}udi_control_t;


typedef struct sdrad_thread_metadata_s{                     
    sdrad_domain_t                active_domain; 
    sti_t                         thread_id;
    int32_t                       sti;
    sdrad_pkru_t                  pkru_config[TOTAL_DOMAIN];  
    pthread_mutexattr_t           attr;    
    pthread_mutex_t               lock;      
    sdrad_d_info_t                sdrad_d_info[TOTAL_DOMAIN]; 
    struct sdrad_thread_metadata_s *next;
}sdrad_thread_metadata_t;


typedef struct{
    struct sdrad_pthread_constructor_s  pthread_constructor_info;
    sdrad_thread_metadata_t             *stm_ptr[NUM_OF_THREADS];   ///mapped thread_id -> thread_index
    sdrad_thread_metadata_t             *stm_head_ptr;  
    sdrad_thread_metadata_t             *stm_root_ptr;      //mapped thread_id -> thread_index
    sdrad_cache_domain_info_t           *sgm_head; 
    sdrad_cache_domain_info_t           *scd_current; 
    udi_t                               sgm_thread_id[NUM_OF_THREADS]; 
    int32_t                             sgm_total_thread; 
    udi_t                               sgm_udi_to_sdi[TOTAL_DOMAIN];
    udi_control_t                       *udi_control_head; 
    udi_t                               sgm_sdi_to_tdi[TOTAL_DOMAIN];
    uint32_t                            status[TOTAL_DOMAIN];  
    int32_t                             pdi[TOTAL_DOMAIN];
    int32_t                             sdi_tdi[TOTAL_DOMAIN];
    sdrad_monitor_domain_t              sdrad_monitor_domain;
    pthread_mutex_t                     sgm_lock;
    pthread_mutexattr_t                 sgm_attr;
    pthread_mutex_t                     sgm_condmutex; 
    pthread_cond_t                      sgm_cond;   
    int32_t                             sdrad_pthread_constructor_done;   
    sighandler_t                        sgm_signal_handler;                     
}__attribute__((packed)) __attribute__((aligned(4096))) sdrad_global_manager_t;


// Monitor Data Domain
extern 
sdrad_global_manager_t sdrad_global_manager __attribute__((aligned(0x1000)));  


// function prototypes of sdrad.c 
extern udi_t 
sdrad_init_destroy(udi_t udi);

extern int32_t 
sdrad_find_free_domain(sdrad_global_manager_t *sgm_ptr);

extern void 
sdrad_get_config(sdrad_d_info_t *sdi_ptr);

extern sdrad_domain_t 
sdrad_domain_init();

extern sdrad_domain_size_t 
sdrad_get_default_stack_size();

extern int32_t 
__sdrad_enter(udi_t udi, void *base_address);

extern void 
__sdrad_enter_root(void *base_address);

extern void
sdrad_enter_root();

extern void 
sdrad_exit_root();

extern int32_t 
sdrad_check_init_config(uint64_t domain_feature);

extern sdrad_domain_t 
sdrad_request_sdi();

extern void 
__sdrad_exit();

extern int32_t 
sdrad_search_udi_control(udi_t udi, udi_control_t  *udi_ptr);

extern tlsf_t *
sdrad_search_tlsf_control(void *ptr, sdrad_global_manager_t *sgm_ptr);

extern void *
sdrad_insert_thread_structure(sdrad_global_manager_t *sgm_ptr);

// extern void *
// sdrad_search_thread_structure();

extern int32_t 
sdrad_abnormal_domain_exit();

extern void
sdrad_delete_control_structure(sdrad_global_manager_t *sgm_ptr, 
                               int32_t sdi);

extern sdrad_domain_t 
sdrad_create_monitor_domain();

void sdrad_data_domain_init(sdrad_global_manager_t *sgm_ptr,
                            sdrad_d_info_t *sdi_ptr, int32_t pkey);


void sdrad_execution_domain_init(sdrad_global_manager_t *sgm_ptr,
                                sdrad_d_info_t *sdi_ptr,     
                                int32_t pkey);

udi_control_t *sdrad_insert_control_structure(sdrad_global_manager_t *sgm_ptr);



                                    
