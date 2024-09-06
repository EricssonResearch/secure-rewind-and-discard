/**
 * @file sdrad_exit_root.S
 * @author Merve Gulmez
 * @brief 
 * @date 2022-02-07
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */
.intel_syntax noprefix
.extern sdrad_exit_root@PLT
#define __ASSEMBLY__

.section .note.GNU-stack,"",@progbits
.section .text
.global  sdrad_exit_root
.type    sdrad_exit_root, @function

sdrad_exit_root: 
    mov     eax, 0
    sub     rsp, 16
    mov     rdi, rsp                // __sdrad_exit() will fill that stack area  
    call    __sdrad_exit_root@PLT
    pop     rdi                     // stack info 
    pop     rax                     // pkru info   
    pop     rsi                     // return address of sdrad_exit() 
    mov     rcx, 0                  // update pkru with parent domain pkru config 
    mov     rdx, 0 
    wrpkru 
    mov     rsp, rdi                // update stack pointer with parent 
    push    rsi
    ret




