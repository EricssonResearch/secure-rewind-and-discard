/**
 * @file sdrad_pkey.h
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
#include <stdint.h>


#define PKEY_DISABLE_ACCESS	            0x1
#define PKEY_DISABLE_WRITE	            0x2
#define PKEY_ACCESS_MASK	            (PKEY_DISABLE_ACCESS | PKEY_DISABLE_WRITE)
// the last 2 bits have to zero, we cannot configure the pkey 0
#define PKRU_ALL_SET                    0xFFFFFFFC   
#define PKRU_ALL_READ_ONLY_RD           0xFFFFFFEC
#define PKRU_ALL_UNSET                  0x00000000 


#define SDRAD_PKRU_SET_DOMAIN(byte,domain,right){\
        uint32_t mask = 3 << (2 * domain);\
        (byte) = (byte& ~mask) | (right << (2 * domain));}\


__attribute__((always_inline))
static inline void  sdrad_store_pkru_config(sdrad_pkru_t rights)
{
    uint32_t eax = rights;
    uint32_t ecx = 0;
    uint32_t edx = 0;

    asm volatile(".byte 0x0f,0x01,0xef\n\t"
            : : "a" (eax), "c" (ecx), "d" (edx));

}


extern void sdrad_pkey_set_domain(int32_t pkey_domain, int32_t parent_domain);

extern int64_t sdrad_pkru_default_config(int32_t pkru_domain); 
extern int64_t sdrad_set_access_pdi(int64_t pkru, 
                                           int32_t pkru_domain, 
                                           int32_t access); 
extern int32_t sdrad_allocate_pkey(sdrad_da_t address, size_t size);