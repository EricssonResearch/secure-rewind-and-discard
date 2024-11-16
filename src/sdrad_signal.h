/**
 * @file sdrad_signal.h
 * @author Merve Gulmez
 * @brief 
 * @version 0.1
 * @date 2023-05-02
 * 
 * @copyright © Ericsson AB 2022-2023
 * 
 * SPDX-License-Identifier: BSD 3-Clause
 */
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
#include <string.h>
#include <pthread.h>

void sdrad_handler(int sig, siginfo_t *si, void *unused);

void sdrad_setup_signal_handler();