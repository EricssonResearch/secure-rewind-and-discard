/**
 * @file global_data_test.c
 * @author Merve Gulmez
 * @brief global data example
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
#include <string.h>
#include "../src/sdrad_api.h"

#define USERBUFSIZE 6 
#define BUFSIZE 100 

int buffer[100]; 
int udi = 1; 


int get_number(void *ptr)
{
    char buf[USERBUFSIZE]; 
    udi = 3;  //it should crash, the nested domain cannot modify the global variable
    strcat(ptr, buf);
    return atoi(buf);
}

int main()
{
    int32_t                            udi, j;
    int sum = 0; 
    int ret; 
    char str[100] = ""; 

    /*Declare udi*/
    udi = 1; 
    for(j=0;j<5; j++)
    {
       if(sdrad_call(udi, &get_number, str, BUFSIZE, &ret) == 
                    SDRAD_SUCCESSFUL_RETURNED){
            sum += ret;
            printf("current story : ");
            puts(str);
       } else {
            printf("Success: Domain Violation Detected\n"); 
       }

   }
}

