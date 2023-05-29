/**
 * @file sdrad_error_handling.c
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
#include <assert.h>

/**
 * @brief the application should be compiled with --Wl,--wrap=__stack_chk_fail
 * it triggers the abnormal domain exit 
 */
void __real___stack_chk_fail(void);

void __wrap___stack_chk_fail(void)
{
    sdrad_abnormal_domain_exit(); 
}
