/**
 * @file sdrad_api.c
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
#include "sdrad_thread.h"
#include "sdrad_heap_mng.h"
#include "sdrad_signal.h"
#include "sdrad_cache.h"
/**
 * @brief 
 * 
 * @param udi 
 * @param source_domain_index 
 * @param access_status 
 * @return int32_t 
 */
int32_t sdrad_dprotect(udi_t udi, udi_t source_domain_index, int32_t access_status)
{
    sdrad_thread_metadata_t     *stm_ptr, *stm_parent_ptr; 
    sdrad_global_manager_t      *sgm_ptr;
    sdrad_d_info_t              *sdi_ptr, *sdi_parent_ptr;
    udi_t                       parent_thread; 
    int32_t                     pdi_source_domain, pdi_parent_domain, pdi_of_udi; 
    int32_t                     sdi, ssdi, active_domain, parent_domain; 


    sdrad_store_pkru_config(PKRU_ALL_UNSET); 
    // Check already initialized or not
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    if(udi == 0){
        sdi = ROOT_DOMAIN;
    }else{
        sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head); 
    }

    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP){
        return SDRAD_ERROR_NO_DOMAIN;
    }
    

    if(source_domain_index == 0){
        ssdi = ROOT_DOMAIN;
    }else{
        ssdi = sdrad_search_udi_control(source_domain_index, sgm_ptr -> udi_control_head); 
    }

    if (ssdi == SDRAD_ERROR_DOMAIN_NO_MAP){
        return SDRAD_ERROR_NO_DOMAIN;
    }


    //Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    assert(thread_index != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    // Get ssdi parent domain 
    // Who initiaziled that domain
    parent_thread = sgm_ptr -> sgm_sdi_to_tdi[ssdi];

    if(parent_thread != stm_ptr -> active_domain){
        stm_parent_ptr = sgm_ptr -> stm_ptr[parent_thread];
        sdi_parent_ptr = (sdrad_d_info_t *)&stm_parent_ptr -> sdrad_d_info[ssdi];
        parent_domain = sdi_parent_ptr -> sdi_parent_domain; 
        sdi_ptr = (sdrad_d_info_t *)&stm_parent_ptr -> sdrad_d_info[ssdi]; 
    }else{
        sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[ssdi];
        parent_domain = sdi_ptr -> sdi_parent_domain; 
        sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[ssdi];
    }

    // What is the pdi of caller domain 
    pdi_parent_domain = sgm_ptr -> pdi[parent_domain];

    // What is the pdi of udi
    pdi_of_udi = sgm_ptr -> pdi[stm_ptr -> active_domain];

    if(pdi_of_udi != pdi_parent_domain)
        return SDRAD_ERROR_WRONG_CONFIG; 

    pdi_source_domain = sgm_ptr -> pdi[ssdi];

    pdi_source_domain = sgm_ptr -> pdi[ssdi];
    /*
    if(!(sdi_ptr -> sdi_options & SDRAD_DATA_DOMAIN)){
       return SDRAD_ERROR_WRONG_CONFIG; 
    } */

    
    stm_ptr -> pkru_config[sdi] = sdrad_set_access_pdi(stm_ptr -> pkru_config[sdi], 
                                                             pdi_source_domain, 
                                                             access_status);

    if(sdi == ROOT_DOMAIN){
        sdrad_store_pkru_config(stm_ptr-> pkru_config[sdi]);
        return SDRAD_SUCCESSFUL_RETURNED; 
    }

    active_domain = stm_ptr -> active_domain;
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return SDRAD_SUCCESSFUL_RETURNED;
}



/**
 * @brief it is called by sdrad_init(). sdrad_init save the sdrad_init_stack_t
 * info
 * 
 * @param base_address  
 * @return int32_t 
 */
int32_t __sdrad_init(void *base_address)
{
    sdrad_d_info_t          *sdi_ptr;
    sdrad_thread_metadata_t *stm_ptr; 
    sdrad_global_manager_t  *sgm_ptr;
    udi_control_t           *udi_ptr;
    sdrad_init_stack_t      *ss_ptr;
    sdrad_pkru_t            pkey; 
    int32_t                 sdi;
    int32_t                 pkru_domain; 

    int32_t                 new_domain; 
    int32_t                 active_domain;

    sdrad_store_pkru_config(PKRU_ALL_UNSET); 
    ss_ptr = (sdrad_init_stack_t *)base_address;

    // Check the options
    if(sdrad_check_init_config(ss_ptr -> domain_feature) == 
                              SDRAD_ERROR_WRONG_CONFIG){
        return SDRAD_ERROR_WRONG_CONFIG; 
    }

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    //Get STI from TDI 
    udi_t thread_index = sdrad_get_sti(sgm_ptr);
    assert(thread_index != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index]; 

    sdi = sdrad_search_udi_control(ss_ptr -> udi, sgm_ptr -> udi_control_head);

 
    // Check already initialized or not
    if (sdi != SDRAD_ERROR_DOMAIN_NO_MAP){
        sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi];

        if(sdi_ptr -> sdi_saved_ex == true){
            return SDRAD_ERROR_DOMAIN_ALREADY_INIT;
        }
        //todo optimization
        memcpy(sdi_ptr->sdi_buffer, ss_ptr -> buffer, sizeof(jmp_buf)); 
        sdi_ptr -> sdi_return_address = ss_ptr -> return_address;
        sdi_ptr -> sdi_parent_domain = stm_ptr -> active_domain;
        sdi_ptr -> sdi_base_address = (sdrad_da_t)base_address;
        sdi_ptr -> sdi_saved_ex = true;
        return SDRAD_SUCCESSFUL_RETURNED;
    }

    // cached domain available or not
    if(sgm_ptr -> sgm_head == NULL){
        // Check pkey available or not
        pkey = pkey_alloc(0,0);
        assert(pkey != -1);
        if (pkey == -1){
            return SDRAD_ERROR_NO_PKEY;
        }
    }else{
        pkey = sgm_ptr -> sgm_head -> scdi_pkey;
    }

    // Map udi to sdi 
    new_domain = sdrad_request_sdi();
    sgm_ptr -> sgm_sdi_to_tdi[new_domain] = thread_index;

    udi_ptr  = sdrad_insert_control_structure(sgm_ptr);
    udi_ptr -> udi = ss_ptr -> udi; 
    udi_ptr -> sdi = new_domain;

    sgm_ptr -> pdi[new_domain] = pkey;

    // Data Domain Create
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[new_domain]; 
    sdi_ptr -> sdi_options = ss_ptr -> domain_feature; 

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[new_domain];
    active_domain = stm_ptr -> active_domain;

    if(ss_ptr -> domain_feature & SDRAD_DATA_DOMAIN){
        sdrad_data_domain_init(sgm_ptr, sdi_ptr, pkey);
        sdi_ptr -> sdi_parent_domain = stm_ptr -> active_domain;
        stm_ptr -> pkru_config[active_domain] = sdrad_set_access_pdi(stm_ptr -> pkru_config[active_domain],
                                                                     pkey, 
                                                                     0);
        sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
        return SDRAD_SUCCESSFUL_RETURNED;
    }

    // Execution Domain Create
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[new_domain];
    sdi_ptr -> sdi_parent_domain = stm_ptr -> active_domain;
    sdi_ptr -> sdi_base_address = (sdrad_da_t)base_address;
    sdrad_execution_domain_init(sgm_ptr, sdi_ptr, pkey); 

    if(ss_ptr -> domain_feature & SDRAD_NONISOLATED_DOMAIN){
        stm_ptr -> pkru_config[active_domain] = sdrad_set_access_pdi(stm_ptr -> pkru_config[active_domain], 
                                                                            pkey, 0);
    }

    pkru_domain = sgm_ptr -> pdi[new_domain]; 

    stm_ptr -> pkru_config[new_domain] = sdrad_pkru_default_config(pkru_domain);

    // no needed to save execution context
    if(ss_ptr -> domain_feature & SDRAD_RETURN_TO_PARENT){
        sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
        return SDRAD_SUCCESSFUL_RETURNED;
    }
    memcpy(sdi_ptr->sdi_buffer, ss_ptr -> buffer, sizeof(jmp_buf)); 
    sdi_ptr->sdi_return_address = ss_ptr -> return_address;
    sdi_ptr -> sdi_saved_ex = true; 
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return SDRAD_SUCCESSFUL_RETURNED;
}



int32_t sdrad_deinit(udi_t udi)
{
    sdrad_d_info_t          *sdi_ptr;
    sdrad_thread_metadata_t *stm_ptr; 
    sdrad_global_manager_t  *sgm_ptr;
    int32_t                 active_domain;
    int32_t                 sdi; 

    sdrad_store_pkru_config(PKRU_ALL_UNSET); 

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    //Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    assert(thread_index != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index]; 

    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head);

    // Check already initialized or not
    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP){
        return SDRAD_ERROR_WRONG_CONFIG;
    }

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi];

    sdi_ptr -> sdi_saved_ex = false; 
    active_domain = stm_ptr -> active_domain;
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return SDRAD_SUCCESSFUL_RETURNED;
}


/* API CALL*/
int32_t sdrad_destroy(udi_t udi, int32_t options)
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_d_info_t               *sdi_ptr, *sdi_parent_ptr;
    sdrad_cache_domain_info_t    *scdi_ptr;
    sdrad_thread_metadata_t      *stm_ptr; 
    int32_t     sdi, active_domain;


    sdrad_store_pkru_config(PKRU_ALL_UNSET); 

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    //Get STI from TDI  
    int32_t thread_index = sdrad_get_sti(sgm_ptr);
    assert(thread_index != -1);  
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head); 

    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP){
        return SDRAD_ERROR_NO_DOMAIN;
    }

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi];
    sdi_parent_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi_ptr -> sdi_parent_domain];
    scdi_ptr = sdrad_insert_cache_domain(sgm_ptr);

    scdi_ptr -> scdi_address_stack = sdi_ptr -> sdi_address_stack; 
    scdi_ptr -> scdi_size_of_stack = sdi_ptr -> sdi_size_of_stack;
    scdi_ptr -> scdi_pkey = sgm_ptr -> pdi[sdi]; 

    if(sdi_ptr -> sdi_init_heap_flag == true){
        if(options == SDRAD_HEAP_MERGE){
            tlsf_merge_pool(sdi_parent_ptr -> sdi_tlsf, sdi_ptr -> sdi_tlsf);
        }else{ 
            scdi_ptr -> scdi_init_heap_domain_flag = true; 
            scdi_ptr -> scdi_address_heap = sdi_ptr -> sdi_address_of_heap; 
            scdi_ptr -> scdi_size_of_heap = sdi_ptr -> sdi_size_of_heap;
            scdi_ptr -> scdi_tlsf = sdi_ptr -> sdi_tlsf;
        }
    }

    // delete udi to sdi map 
    sgm_ptr -> sgm_udi_to_sdi[sdi] = 0; 

    // delete pdi to sdi map
    sgm_ptr -> pdi[sdi] = 0;
    sdrad_delete_control_structure(sgm_ptr, sdi); 
    // restore PKU config
    active_domain = stm_ptr -> active_domain;
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return SDRAD_SUCCESSFUL_RETURNED;
}



void __sdrad_exit(void *base_address)
{
    sdrad_thread_metadata_t      *stm_ptr; 
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_d_info_t               *sdi_ptr;
    sdrad_d_info_t               *sdi_parent_ptr;
    int32_t                      active_domain; 
    int32_t                      parent_domain;
    int32_t                      sti;
    sdrad_ee_stack_t  *ss_ptr; 


    sdrad_store_pkru_config(PKRU_ALL_UNSET);
    ss_ptr = (sdrad_ee_stack_t *)base_address;
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    // Get STI from TDI 
    sti = sdrad_get_sti(sgm_ptr); 
    assert(sti != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[sti];

    // Get the active domain 
    active_domain = stm_ptr -> active_domain; 


    // Get its parent domain 
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[active_domain];
    parent_domain = sdi_ptr -> sdi_parent_domain; 
    sdi_parent_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[parent_domain];

    // Set active domain with parent domain
    stm_ptr -> active_domain = sdi_ptr -> sdi_parent_domain; 
    tlsf_structure = sdi_parent_ptr -> sdi_tlsf;
    
    ss_ptr -> stack_info = (int64_t)(sdi_parent_ptr -> sdi_last_rsp); 
    ss_ptr -> pkru_info = stm_ptr -> pkru_config[parent_domain];
}





int32_t __sdrad_enter(udi_t udi, void *base_address)
{
    sdrad_d_info_t              *sdi_ptr, *sdi_parent_ptr;
    sdrad_thread_metadata_t     *stm_ptr; 
    sdrad_global_manager_t      *sgm_ptr;
    int32_t                     sdi, parent_sdi;
    sdrad_ee_stack_t            *ss_ptr; 

    sdrad_store_pkru_config(PKRU_ALL_UNSET);
    ss_ptr = (sdrad_ee_stack_t *)base_address;
    // Check already initialized or not
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;

    // Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    assert(thread_index != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head); 

    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP)
        return SDRAD_ERROR_NO_DOMAIN;

    // Check the config, we cannot enter the data domain
    if(stm_ptr -> sdrad_d_info[sdi].sdi_options & SDRAD_DATA_DOMAIN){
        return SDRAD_ERROR_WRONG_CONFIG;
    }

    // Check no execution context
    if(stm_ptr -> sdrad_d_info[sdi].sdi_saved_ex == false){
        return SDRAD_ERROR_WRONG_CONFIG;
    }

  
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi]; 
    parent_sdi = sdi_ptr -> sdi_parent_domain; 
    tlsf_structure = sdi_ptr -> sdi_tlsf;
    sdi_parent_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[parent_sdi];

    stm_ptr -> active_domain = sdi;
    /* parent domain stack pointer doesn't cover current return address */
    sdi_parent_ptr -> sdi_last_rsp = (sdrad_da_t)ss_ptr -> stack_info + 16;    
    ss_ptr -> stack_info = (int64_t)(sdi_ptr -> sdi_address_stack + sdi_ptr -> sdi_size_of_stack - sdi_ptr -> sdi_address_offset_stack); 
    ss_ptr -> pkru_info = stm_ptr -> pkru_config[sdi];
    return sdi; 
}



int32_t sdrad_call(udi_t udi, 
              int32_t (*funcptr)(),  
              void *argument, 
              size_t argument_size, 
              int32_t *ret_p)
{
    register void *ptr_val             asm ("r15");
    register int32_t  ret_val          asm ("r11");
    int32_t                                 err;


    // Init Domain
    err = sdrad_init(udi, SDRAD_EXECUTION_DOMAIN | SDRAD_NONISOLATED_DOMAIN); 

    if(err == SDRAD_SUCCESSFUL_RETURNED){
        ptr_val = sdrad_malloc(udi, argument_size); 
        memcpy(ptr_val, argument, argument_size);

        sdrad_enter(udi); 
        ret_val = funcptr(ptr_val);
        sdrad_exit(); 

        memcpy(argument, ptr_val, argument_size);

        *ret_p = ret_val;
        sdrad_destroy(udi, SDRAD_NO_HEAP_MERGE); 

    }
    return err;
}

long sdrad_get_stack_offset(uint64_t udi)
{
    sdrad_global_manager_t      *sgm_ptr;
    int32_t                     sdi;
    sdrad_d_info_t              *sdi_ptr; 
    sdrad_thread_metadata_t     *stm_ptr; 
    int32_t                      active_domain; 
    int32_t                      sti;
    int64_t                      rsp; 

    sdrad_store_pkru_config(PKRU_ALL_UNSET);
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;

    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head);

    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP)
        return SDRAD_ERROR_NO_DOMAIN;

    // Get STI from TDI 
    sti = sdrad_get_sti(sgm_ptr); 
    assert(sti != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[sti];

    active_domain = stm_ptr -> active_domain; 

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi]; 
    rsp = sdi_ptr -> sdi_address_stack + sdi_ptr -> sdi_size_of_stack; 
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 

    return rsp; 
}


long sdrad_set_stack_offset(uint64_t udi, long offset)
{
    sdrad_global_manager_t      *sgm_ptr;
    int32_t                     sdi;
    sdrad_d_info_t              *sdi_ptr; 
    sdrad_thread_metadata_t     *stm_ptr; 
    int32_t                      sti;
    int32_t                      active_domain; 

    sdrad_store_pkru_config(PKRU_ALL_UNSET);
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager;

    sdi = sdrad_search_udi_control(udi, sgm_ptr -> udi_control_head);

    if (sdi == SDRAD_ERROR_DOMAIN_NO_MAP)
        return SDRAD_ERROR_NO_DOMAIN;

         // Get STI from TDI 
    sti = sdrad_get_sti(sgm_ptr); 
    assert(sti != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[sti];

    active_domain = stm_ptr -> active_domain; 

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[sdi]; 

    sdi_ptr -> sdi_address_offset_stack = offset;  
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
    return SDRAD_SUCCESSFUL_RETURNED;
}