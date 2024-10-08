/**
 * @file sdrad_enter_root.S
 * @author Merve Gulmez
 * @brief 
 * @date 2022-02-07
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */
.intel_syntax noprefix
#define __ASSEMBLY__
.section .note.GNU-stack,"",@progbits
.section .text
.global  sdrad_enter_root
.type    sdrad_enter_root, @function
/*

struct sdrad_ee_stack_t
{
    long stack_info;
    long pkru_info;
}__attribute__((packed));
*/
sdrad_enter_root:
    sub     rsp, 8              // pkru_info it is saved by __sdrad_enter
    push    rsp                 // parent domain stack will be saved + 16 
    mov     rdi, rsp            // base address of sdrad_init_stack_t
    call    __sdrad_enter_root@PLT   // it saves the parent domain stack + returns child domain stack */ 
    pop     rdi                 //  child domain stack
    pop     rsi                 //  pkru config 
    pop     rax                 //  save the return address 
    mov     rsp, rdi            //  change stack 
    push    rax                 //  push return address to new stack
    mov     rax, rsi            //  update pkru config 
    mov     rcx, 0
    mov     rdx, 0 
    wrpkru 
    ret






