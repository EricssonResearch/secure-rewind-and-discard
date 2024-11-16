/**
 * @file sdrad_heap_mng.h
 * @author Merve Gulmez
 * @brief 
 * @version 0.1
 * @date 2023-05-02
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



extern void sdrad_heap_domain_init(sdrad_d_info_t *sdi_ptr, int32_t pkey);
extern void *sdrad_malloc_mdd(size_t size);
extern void free(void *ptr);
extern void *malloc(size_t size);
extern void *calloc(size_t nelem, size_t elsize);
extern void *realloc(void *ptr, size_t size);

