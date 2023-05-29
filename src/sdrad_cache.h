/**
 * @file sdrad_cache.h
 * @author Merve Gulmez
 * @brief 
 * @version 0.1
 * @date 2023-05-01
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */
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


extern void *sdrad_insert_cache_domain(sdrad_global_manager_t *sgm_ptr); 

extern void sdrad_delete_cache_domain(sdrad_global_manager_t *sgm_ptr);


