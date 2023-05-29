/**
 * @file stackoverflow_handler.c
 * @author Merve Gulmez
 * @brief Simple Example
 * @version 0.1
 * @date 2023-05-17
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
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include "../src/sdrad_api.h"


void get_request(); 

void get_request()
{
    char buf[4];
    printf("WAITING REQUEST \n"); 
    gets(buf); 
    printf("HANDLING REQUEST \n"); 
}

void main()
{
    int32_t                            err;
    int32_t                            udi, j;

    /*Declare udi*/
    udi = 1; 
    for(j = 0; j < 5; j++) {
        /*Domain Init */
        err = sdrad_init(udi, SDRAD_EXECUTION_DOMAIN | SDRAD_NONISOLATED_DOMAIN); 
        if(err == SDRAD_SUCCESSFUL_RETURNED || err == SDRAD_WARNING_SAVE_EC){ /*Normal Domain Exit*/
            sdrad_enter(udi); 
            get_request(); 
            sdrad_exit(); 
            sdrad_deinit(udi); 
        } else {
            printf("Success: Domain Violation Detected\n"); /*Abnormal Domain Exit*/
        }
    }
}

