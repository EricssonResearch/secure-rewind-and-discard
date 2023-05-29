/**
 * @file sdrad_signal.c
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
#include <stdbool.h>
#include <assert.h>
#include <sys/resource.h> 
#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include "/usr/include/event.h"
#include "sdrad.h"
#include "sdrad_pkey.h"
#include "tlsf.h" 
#include "sdrad_heap_mng.h"
#include "sdrad_thread.h"




void __sdrad_signal_handler(int32_t sig, siginfo_t *si, void *unused)
{  
    // sdrad_thread_metadata_t      *stm_ptr;
    // sdrad_d_info_t               *sdi_ptr; 
    // sdrad_global_manager_t       *sgm_ptr;
    // sdrad_d_info_t               *sdi_ptr_1;
    // int32_t                      parent_domain;
 

    // sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 

    //clock_gettime(CLOCK_REALTIME, &start); 

    //Get STI from TDI 
    // int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    // assert(thread_index != -1); 
    // stm_ptr = sgm_ptr -> stm_ptr[thread_index];
    
    // sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[stm_ptr -> active_domain]; 
  /*
    __asm__ volatile( 
        "mov %0, %%rsp\n": 
        : "r"(sdi_ptr -> sdi_address_stack): 
    ); */
    sigset_t sigset;
    int32_t ret;
    ret = sigemptyset(&sigset); assert(0 == ret);
    ret = sigaddset(&sigset, SIGSEGV); assert(0 == ret);
    ret = sigprocmask(SIG_UNBLOCK, &sigset, NULL); assert(0 == ret);
    sdrad_abnormal_domain_exit();
}


// void __sdrad_signal(int32_t sig, siginfo_t *si, void *unused)
// {  
//     sdrad_thread_metadata_t      *stm_ptr;
//     sdrad_d_info_t               *sdi_ptr; 
//     sdrad_global_manager_t       *sgm_ptr;
//     sdrad_d_info_t               *sdi_ptr_1;
//     int32_t                      parent_domain;
//     void                         (*fun_ptr)(); 
//     int32_t                      active_domain;
 

//     sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 


//     //Get STI from TDI 
//     int32_t thread_index = sdrad_get_sti(sgm_ptr); 
//     assert(thread_index != -1); 
//     stm_ptr = sgm_ptr -> stm_ptr[thread_index];

//     sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[stm_ptr -> active_domain]; 

//     fun_ptr = sgm_ptr -> sgm_signal_handler;

//     fun_ptr(); 

//     // sigset_t sigset;
//     // // int32_t ret;
//     // // ret = sigemptyset(&sigset); assert(0 == ret);
//     // // ret = sigaddset(&sigset, sig); assert(0 == ret);
//     // // ret = sigprocmask(SIG_UNBLOCK, &sigset, NULL); assert(0 == ret);
//     // // restore PKU config
//     // active_domain = stm_ptr -> active_domain;
//     // sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
// }


/*
sighandler_t  (*real_signal)(); 
sighandler_t  signal(int signum, sighandler_t  handler)
{
    sdrad_thread_metadata_t      *stm_ptr;
    sdrad_d_info_t               *sdi_ptr; 
    sdrad_global_manager_t       *sgm_ptr;
    int32_t                      parent_domain, active_domain;

    sdrad_store_pkru_config(PKRU_ALL_UNSET);

    if(real_signal == NULL) {
        real_signal = dlsym(RTLD_NEXT, "signal");
        if (NULL == real_signal) {
            fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
        }
    }

    sgm_ptr = (sdrad_global_manager_t *)&sdrad_global_manager; 
    
    //Get STI from TDI 
    int32_t thread_index = sdrad_get_sti(sgm_ptr); 
    assert(thread_index != -1); 
    stm_ptr = sgm_ptr -> stm_ptr[thread_index];

    sdi_ptr = (sdrad_d_info_t *)&stm_ptr -> sdrad_d_info[stm_ptr -> active_domain]; 
    sgm_ptr -> sgm_signal_handler = handler;
    real_signal(signum, sdrad_signal); 

    // restore PKU config
    active_domain = stm_ptr -> active_domain;
    sdrad_store_pkru_config(stm_ptr-> pkru_config[active_domain]); 
}

*/
static  void __attribute__((naked)) sdrad_signal_handler(/*sig, info, ucontext*/) 
{
    asm(             
      "xor %eax, %eax\n" // clear eax (full permission)
      "xor %ecx, %ecx\n" // clear ecx (needed for wrpkru)
      "xor %edx, %edx\n" // clear edx (needed for wrpkru)
      "wrpkru\n"
      "jmp __sdrad_signal_handler@PLT\n"
    );

}



void sdrad_setup_signal_handler()
{
    struct sigaction sa;
    
    sa.sa_flags =  SA_SIGINFO ;
    sa.sa_handler = sdrad_signal_handler;     
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        SDRADEXIT("sigaction");
    }

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        SDRADEXIT("sigaction");
    }
}
