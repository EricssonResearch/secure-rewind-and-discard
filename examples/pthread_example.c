/**
 * @file pthread_example.c
 * @author Merve Gulmez
 * @brief global data example
 * @version 0.1
 * @date 2023-05-17
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../src/sdrad_api.h"


// Global variable:
int i = 2;

void get_request(); 
void get_request(int *a)
{
    *a = 5; // it should crash!!! 
}

void *foo()
{ 
    int32_t                            err;
    int                                a, j; 
    
    a = 3; 
    /*Declare udi*/
    int udi = 3; 
    for(j = 0; j < 5; j++)
    {
        /*Domain Init */
        err = sdrad_init(udi, SDRAD_EXECUTION_DOMAIN | SDRAD_NONISOLATED_DOMAIN); 
        if(err == SDRAD_SUCCESSFUL_RETURNED || err == SDRAD_WARNING_SAVE_EC){ /*Normal Domain Exit*/
            sdrad_enter(udi); 
            get_request(&a); 
            sdrad_exit(); 
            sdrad_deinit(udi); 
        } else {
            printf("Success: Domain Violation Detected\n"); /*Abnormal Domain Exit*/
        }
    }
    pthread_exit(NULL);
}


int main()
{
    // Declare variable for thread's ID:
    pthread_t id;
    pthread_create(&id, NULL, foo, NULL);

    // Wait for foo() and retrieve value in ptr;
    pthread_join(id, NULL);
}


