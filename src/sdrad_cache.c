/**
 * @file sdrad_cache.c
 * @author Merve Gulmez 
 * @brief Destroyed domain is never unmapped, they are cached. 
 * @version 0.1
 * @date 2023-05-01
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
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "/usr/include/event.h"
#include <sys/resource.h> 
#include <linux/unistd.h>
#include <asm/ldt.h>
#include <assert.h>
#include "sdrad.h"
#include "sdrad_pkey.h"
#include "sdrad_api.h"
#include "tlsf.h" 
#include "sdrad_heap_mng.h"
#include "sdrad_thread.h"
#include "sdrad_signal.h"





void *sdrad_insert_cache_domain(sdrad_global_manager_t       *sgm_ptr)
{
    sdrad_cache_domain_info_t     *scdi_ptr; 

    scdi_ptr = sdrad_malloc_mdd(sizeof(sdrad_cache_domain_info_t));
    //point it to old first node
    scdi_ptr -> scdi_next = sgm_ptr -> sgm_head;
    //point first to new first node
    sgm_ptr -> sgm_head = scdi_ptr;
    return scdi_ptr;
}

void sdrad_delete_cache_domain(sdrad_global_manager_t       *sgm_ptr)
{
    //sdrad_cache_domain_info_t     *scdi_temp_ptr; 

    //scdi_temp_ptr = sgm_ptr -> sgm_head;
    sgm_ptr -> sgm_head = sgm_ptr -> sgm_head -> scdi_next;
}