/**
 * @file sdrad_enter.S
 * @author Merve Gulmez
 * @brief 
 * @date 2022-02-07
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */

.intel_syntax noprefix
.extern  _setjmp@PLT
#define __ASSEMBLY__

.section .text
        .global  sdrad_init


.type    sdrad_init, @function

/*
sdrad_init_destroy saves the return address"
                   saves the pkru config"

struct
{
struct sdrad_init_stack_t
{
    jmp_buf buffer;
    long domain_feature;
    long udi;
    long return_address;
}__attribute__((packed));
}
*/
sdrad_init:
    push    rdi                 //      save udi
    push    rsi                 //      save domain flag 
    sub     rsp, 200            //      sizeof(jmp_buffer) = 200 
    mov     rdi, rsp            //      setjmp buffer parameter push the rdi
    call    _setjmp@PLT
    test    eax, eax            //      we excepted that longjmp trigger
    je     .L4
    
    call    sdrad_init_destroy@PLT 
    add     rsp, 208             // 
    pop     rax                  //     pkru_info
    mov     rcx, 0
    mov     rdx, 0 
    wrpkru                        //    224
    ret 

   

.L4:
    mov     rdi, rsp                          
    call    __sdrad_init@PLT 
    add     rsp, 216                
    ret