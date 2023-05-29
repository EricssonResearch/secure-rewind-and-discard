/**
 * @file sdrad_call_example.c
 * @author Merve Gulmez
 * @brief sdrad_call() example
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
int udi = 1; 


int get_number(void *ptr)
{
    char buf[USERBUFSIZE]; 
    printf("Enter Keyword \n"); 
    gets(buf); 
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
    for(j = 0; j < 5; j++)
    {
       if(sdrad_call(udi, &get_number, str, BUFSIZE, &ret) == 
                    SDRAD_SUCCESSFUL_RETURNED){
            sum += ret;
            printf("Current Buf : ");
            puts(str);
       } else {
            printf("ERROR_BAD_INPUT \n"); 
       }

   }
}

