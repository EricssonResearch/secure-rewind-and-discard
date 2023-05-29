/**
 * @file sdrad_pkey.c
 * @author Merve Gulmez
 * @brief 
 * @version 0.1
 * @date 2023-05-02
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */
#define _GNU_SOURCE
#include <unistd.h>
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
#include <string.h>
#include <pthread.h>
#include <sys/resource.h> 
#include "sdrad.h"
#include "tlsf.h" 
#include "sdrad_pkey.h"
#include <assert.h> 

/**
 * @brief 
 * 
 * @param pkru_domain : it should be pkru domain, not sdrob domain
 */

int64_t sdrad_pkru_default_config(int32_t pkru_domain)
{
    int64_t pkru = PKRU_ALL_READ_ONLY_RD; 
    SDRAD_PKRU_SET_DOMAIN(pkru, pkru_domain, 0); 
    return pkru; 
}

int64_t sdrad_set_access_pdi(int64_t pkru, 
                                    int32_t pkru_domain, 
                                    int32_t access)
{
    SDRAD_PKRU_SET_DOMAIN(pkru, pkru_domain, access); 
    return pkru; 
}


int32_t sdrad_allocate_pkey(sdrad_da_t  address, size_t size)
{
    int32_t     status; 
    int32_t     pkey; 

    pkey = pkey_alloc(0,0);
    assert(pkey != -1);
    status = pkey_mprotect((void *)address,
                            size, 
                            PROT_READ | PROT_WRITE, 
                            pkey);  

    assert(status != -1);
    return pkey;
}


