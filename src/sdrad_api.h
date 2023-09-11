/**
 * @file sdrad_api.h
 * @author Merve Gulmez
 * @brief Developers use the SDRaD API calls
 * to flexibly enhance their application with a secure rewind
 * mechanism, accounting for the design patterns
 * @version 0.1
 * @date 2022-02-07
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

/// Memcached CVE TIME STARTING/// 
extern struct timespec start, stop;
extern double accum;
#define BILLION 1E9


// sdrad_init Return Value
#define SDRAD_SUCCESSFUL_RETURNED       -1 
#define SDRAD_ERROR_NO_PKEY             -2
#define SDRAD_ERROR_DOMAIN_ALREADY_INIT -3
#define SDRAD_ERROR_WRONG_CONFIG        -4 
#define SDRAD_WARNING_SAVE_EC           -5
#define SDRAD_ERROR_NO_DOMAIN           -6
#define SDRAD_ERROR_NO_EC               -7

#define  SDRAD_ERROR_DOMAIN_NO_MAP      -20


// Init Flag 
#define SDRAD_EXECUTION_DOMAIN      (1 << 0)	
#define SDRAD_DATA_DOMAIN  	        (1 << 1)
#define SDRAD_ISOLATED_DOMAIN  	    (1 << 2)
#define SDRAD_NONISOLATED_DOMAIN  	(1 << 3)
#define SDRAD_RETURN_TO_PARENT  	(1 << 4)	
#define SDRAD_RETURN_TO_CURRENT   	(1 << 6)
#define SDRAD_NO_HEAP_MERGE   	    (1 << 7)
#define SDRAD_HEAP_MERGE   	        (1 << 8)




/**
* @brief sdrad_init: Initialize Domain udi
* 1) Domain can be Execution Domain or Data domain.  
*       1.1) Execution Domain has own stack and heap area. 
*              1.1.1) On an abnormal domain exit, sdrad_init() returns with udi 
*                     to the parent domain  
*                     or currently executed domain depending on 
*                     SDRAD_RETURN_TO_PARENT/SDRAD_RETURN_TO_CURRENT 
*                     flag respectively. 
*              1.1.2) The newly created domain can be fully isolated 
*                     from the parent domain or 
*                     non-isolated domain using 
*                     SDRAD_ISOLATED_DOMAIN/SDRAD_NONISOLATED_DOMAIN 
*              1.2) Data Domain does have a separate subheap area. 
*                   It can be used to protect a database or 
*                   data passing of between domains. 
*                   
 * @param udi: user domain index
 *             it should be unique value for different domains. 

 * @param domain_feature flag 
                     SDRAD_DATA_DOMAIN  	       
                     SDRAD_ISOLATED_DOMAIN  	 
                     SDRAD_NONISOLATED_DOMAIN  	
                     SDRAD_RETURN_TO_PARENT   
                     SDRAD_RETURN_TO_CURRENT  
 * @return OK  : SDRAD_SUCCESSFUL_RETURNED (-1) 
           FAIL: SDRAD_NO_PKEY_AVAILABLE (-2)
               : SDRAD_DOMAIN_ALREADY_INIT (-3)
               : In case of abnormal_domain_exit: return udi 
 */
int32_t sdrad_init(uint64_t udi, long domain_feature_flag);



/**
 * @brief entering already initialized domain
 * 
 * @param udi 
 * @return int32_t SDRAD_ERROR_NO_DOMAIN 
 *                 SDRAD_ERROR_WRONG_CONFIG
 */
int32_t sdrad_enter(uint64_t udi);



/**
 * @brief exiting the current domain to parent domain
 * 
 */
void sdrad_exit();



/**
 * @brief Set domain udi’s access permissions
 * to PROT on target data domain udi 
 * 
 * @param target_domain_index 
 * @param source_domain_index 
 * @param access_status 
 * @return int32_t  SDRAD_ERROR_NO_DOMAIN
 *                  SDRAD_ERROR_WRONG_CONFIG
 *                  SDRAD_SUCCESSFUL_RETURNED
 * 
 */
 int32_t sdrad_dprotect(uint64_t target_domain_index, 
                    uint64_t source_domain_index, 
                    int32_t access_status);




/**
 * @brief Destroy Child Domain with udi
 * 
 * @param udi 
 * @param options  SDRAD_NO_HEAP_MERGE
 *                 SDRAD_HEAP_MERGE
 * @return int32_t  SDRAD_ERROR_NO_DOMAIN
 *                  SDRAD_SUCCESSFUL_RETURNED
 */
 int32_t sdrad_destroy(uint64_t udi, int32_t options);




/**
 * @brief Delete return context of domain udi
 * 
 * @param udi 
 * @return int32_t SDRAD_ERROR_WRONG_CONFIG
 *                 SDRAD_SUCCESSFUL_RETURNED
 */
 int32_t sdrad_deinit(uint64_t udi);




/* Memory Management API Call */
void *sdrad_malloc(uint64_t udi, size_t size);

void *sdrad_calloc(uint64_t udi, 
                   size_t nelem, size_t elsize);

void sdrad_free(uint64_t udi, void *ptr);

void *sdrad_realloc(uint64_t udi, void *ptr, size_t size);

int sdrad_memalign(uint64_t udi, void **memptr, size_t alignment, size_t size);


/**
 * @brief Convenience wrapper for single
*  size, ret function calls with one argument
 * 
 * @param function_index 
 * @param funcptr 
 * @param argument_size 
 * @return int32_t  OK  : SDRAD_SUCCESSFUL_RETURNED (-1) 
           FAIL: SDRAD_NO_PKEY_AVAILABLE (-2)
               : SDRAD_DOMAIN_ALREADY_INIT (-3)
               : In case of abnormal_domain_exit: return udi 
 */
int32_t sdrad_call(uint64_t udi, int32_t (*funcptr)(),  void *argument, 
               size_t argument_size, int32_t *ret);


/**
 * @brief RUST sandbox! can get the stack-offset 
 * 
 * @param udi 
 * @return stack address
 */
long sdrad_get_stack_offset(uint64_t udi); 

/**
 * @brief RUST sandbox! can set the stack-offset 
 * 
 * @param udi 
 * @return stack address
 */
long sdrad_set_stack_offset(uint64_t udi, long rsp); 

