/**
 * @file sdrad_thread.c
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
#include <pthread.h>
#include <dlfcn.h>
#include "/usr/include/event.h"
#include <sys/resource.h> 
#include <linux/unistd.h>
#include <string.h>
#include <asm/ldt.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include "tlsf.h" 
#include "sdrad.h"
#include "sdrad_pkey.h"
#include "sdrad_heap_mng.h"
#include "sdrad_thread.h"






void sdrad_signal_cond()
{
    assert(0 == pthread_cond_signal(&sdrad_global_manager.sgm_cond));
}

void sdrad_wait_cond() 
{

    assert(0 == pthread_mutex_lock(&sdrad_global_manager.sgm_condmutex));
    assert(0 == pthread_cond_wait(&sdrad_global_manager.sgm_cond, 
                                  &sdrad_global_manager.sgm_condmutex));
    assert(0 == pthread_mutex_unlock(&sdrad_global_manager.sgm_condmutex));

}

void sdrad_mutex_constructor(sdrad_global_manager_t *sgm_ptr)
{

    pthread_mutexattr_init(&sgm_ptr->sgm_attr);
    pthread_mutexattr_settype(&sgm_ptr->sgm_attr,  PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&sgm_ptr->sgm_lock, &sgm_ptr->sgm_attr); 
    pthread_mutex_init(&sgm_ptr->sgm_condmutex, NULL);
    pthread_cond_init(&sgm_ptr->sgm_cond, NULL); 

}


/**
 * @brief  before start routine functions
 * 
 * @return void* 
 */
void *sdrad_pthread_construstor()
{
    sdrad_global_manager_t       *sgm_ptr;
    sdrad_d_info_t               *sdi_ptr;
    sdrad_thread_metadata_t      *stm_ptr;
    pthread_attr_t               attr;
    void                         (*fun_ptr)(); 
    int32_t                      status;
    int32_t                      active_domain;
    void                         *arg;
    size_t                       guard_size;
    int64_t                      root_domain_pkru;
 
    sdrad_store_pkru_config(PKRU_ALL_UNSET); 
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    // Mapping TID to STI 
    sgm_ptr -> sgm_thread_id[sgm_ptr -> sgm_total_thread] = pthread_self();
    // Get memory area from Monitor Data Domain 
    sgm_ptr -> stm_ptr[sgm_ptr -> sgm_total_thread] = sdrad_malloc_mdd(sizeof(sdrad_thread_metadata_t));
    stm_ptr = sgm_ptr -> stm_ptr[sgm_ptr -> sgm_total_thread];
    sgm_ptr -> sgm_total_thread ++;  

    /* Assign a SDI domain */
    stm_ptr -> active_domain = sdrad_request_sdi();

    SDRAD_DEBUG_PRINT("SDRAD THREAD DOMAIN %d ", stm_ptr -> active_domain); 

    active_domain = stm_ptr -> active_domain;
    sdi_ptr = (sdrad_d_info_t *)&stm_ptr->sdrad_d_info[active_domain];
    fun_ptr = sgm_ptr -> pthread_constructor_info.real_start_routine; 
    arg = sgm_ptr -> pthread_constructor_info.arg;
    /* after saving function pointer and arg to thread local data, 
    * send the signal to __wrap_pthread_create function */
    sgm_ptr -> sdrad_pthread_constructor_done = 1;
    sdrad_signal_cond();
    /* getting thread stack and size */
    pthread_getattr_np(pthread_self(), &attr);
    pthread_attr_getguardsize(&attr, &guard_size);
    pthread_attr_getstack(&attr, 
                          (void *)&sdi_ptr -> sdi_address_stack, 
                          &sdi_ptr -> sdi_size_of_stack);

    SDRAD_DEBUG_PRINT("pthread stack: %p-%p (len = %zu = 0x%zx) \n", 
                        sdi_ptr -> sdi_address_stack, 
                        sdi_ptr -> sdi_address_stack + sdi_ptr -> sdi_size_of_stack, 
                        sdi_ptr -> sdi_size_of_stack, 
                        sdi_ptr -> sdi_size_of_stack);

    sdi_ptr -> sdi_address_stack += 1*PAGE_SIZE;
    sdi_ptr -> sdi_size_of_stack -= 2*PAGE_SIZE; 
    ///BUG THERE??? it is not related to current problem
  
    SDRAD_MUTEX_LOCK();
#ifdef SDRAD_STRONG_MULTITHREAD
    sgm_ptr -> pdi[stm_ptr -> active_domain] = pkey_alloc(0,0); 
    assert(sgm_ptr -> pdi[stm_ptr -> active_domain ] == -1)

    pkey_mprotect((void *)sdi_ptr -> sdi_address_stack, 
                          PROT_READ | PROT_WRITE | PROT_GROWSUP, 
                          sdi_ptr -> sdi_size_of_stack, 
                          sgm_ptr -> pdi[stm_ptr -> active_domain);

    stm_ptr -> pkru_config[stm_ptr -> active_domain] = sdrad_pkru_default_config(sgm_ptr -> pdi[stm_ptr -> active_domain]);
#else
    status = pkey_mprotect((void *)sdi_ptr -> sdi_address_stack, 
                            sdi_ptr -> sdi_size_of_stack, 
                            PROT_READ | PROT_WRITE, 
                            sgm_ptr -> pdi[ROOT_DOMAIN]); 

    assert(status != -1);
    sgm_ptr -> pdi[stm_ptr -> active_domain] = sgm_ptr -> pdi[ROOT_DOMAIN];
    root_domain_pkru = sgm_ptr -> stm_ptr[ROOT_DOMAIN] -> pkru_config[ROOT_DOMAIN]; 
    stm_ptr -> pkru_config[stm_ptr -> active_domain] =  root_domain_pkru;
#endif
    SDRAD_MUTEX_UNLOCK();
    tlsf_structure = sdi_ptr -> sdi_tlsf; 
    sdrad_store_pkru_config(stm_ptr -> pkru_config[stm_ptr -> active_domain]); 
    fun_ptr(arg);
}

static int32_t (*real_pthread_create)(pthread_t *thread, 
                              const pthread_attr_t *attr,
                              void *(*start_routine) (void *), 
                              void *arg) = NULL;

void pthread_load_sym()
{
    void *libc_handle, *sym;
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 35)
    libc_handle = dlopen("libc.so.6", RTLD_NOLOAD | RTLD_NOW);
#else
    libc_handle = dlopen("libpthread.so.0", RTLD_NOLOAD | RTLD_NOW);
#endif
	

	if (!libc_handle) {
		fprintf(stderr, "can't open handle to libc.so.6: %s\n",
			dlerror());
		_exit(EXIT_FAILURE);
	}
	
	sym = dlsym(libc_handle, "pthread_create");
	if (!sym) {
		fprintf(stderr, "can't find __libc_start_main():%s\n",
			dlerror());
		_exit(EXIT_FAILURE);
	}

	real_pthread_create = sym;
	
	if(dlclose(libc_handle)) {
		fprintf(stderr, "can't close handle to libc.so.6: %s\n",
			dlerror());
		_exit(EXIT_FAILURE);
	}
}




int32_t pthread_create(pthread_t *thread, 
                             const pthread_attr_t *attr,
                             void *(*start_routine) (void *), 
                             void *arg)
{
    sdrad_global_manager_t              *sgm_ptr;
    struct sdrad_pthread_constructor_s  *spc_ptr;
    int32_t ret;

    sdrad_store_pkru_config(PKRU_ALL_UNSET); 
    if (real_pthread_create == NULL)
        pthread_load_sym(); 
    SDRAD_MUTEX_LOCK(); 
    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    assert(0 == sgm_ptr -> sdrad_pthread_constructor_done);
    // saving arg and func value to global data mutex already locked 
    spc_ptr = (struct sdrad_pthread_constructor_s *)&sgm_ptr -> pthread_constructor_info; 
    spc_ptr -> real_start_routine = start_routine;
    spc_ptr -> arg = arg;
    // mutex should wait until constructor function is done. 
    sgm_ptr -> sdrad_pthread_constructor_done = 0;
    ret = real_pthread_create(thread, attr, sdrad_pthread_construstor, arg);
    sdrad_store_pkru_config(PKRU_ALL_UNSET); 
    while (sgm_ptr -> sdrad_pthread_constructor_done == 0) {
        sdrad_wait_cond();
    }
    assert(1 == sgm_ptr -> sdrad_pthread_constructor_done);
    sgm_ptr -> sdrad_pthread_constructor_done = 0;
    SDRAD_MUTEX_UNLOCK();
    return ret;
}


