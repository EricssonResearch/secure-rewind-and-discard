# Secure Rewind & Discard of Isolated Domains

This repository contains the source code for the paper [Rewind & Discard: Improving Software Resilience using Isolated Domains](https://arxiv.org/pdf/2205.03205.pdf)

## Abstract

Well-known defenses exist to detect and mitigate common faults and memory safety
vulnerabilities in software. Yet, many of these mitigations do not address the
challenge of software resilience and availability, i.e., whether a
system can continue to carry out its function and remain responsive, while being
under attack and subjected to malicious inputs. In this paper we propose
secure rewind and discard of isolated domains as an efficient and secure
method of improving the resilience of software that is targeted by run-time
attacks. In difference to established approaches, we rely on
compartmentalization instead of replication and checkpointing. We show the
practicability of our methodology by realizing a software library for
Secure Domain Rewind and Discard SDRaD and demonstrate how SDRaD can be applied
to real-world software.

## How to get started

- This repository can be cloned using the following commands:
```
git clone git@github.com:EricssonResearch/secure-rewind-and-discard.git
```
- To compile SDRaD, Run `make` in [src/](./src/). 

```
cd ./secure-rewind-and-discard/src
make
```
Please see [src/README.md](./src/README.md) for the different compilation and run-time options supported by SDRaD.

Before running applications that rely on the SDRaD library (`libsdrad.so`) add the SDRaD [src/](./src/) directory containing the shared object to the Linux dynamic linker search path:

```
export LD_LIBRARY_PATH='/path/to/secure-rewind-and-discard/src'
```

If the application relies on pre-built binaries which make calls to the `malloc()` family of functions, you additionally need to ensure that `libsdrad.so` is loaded before all other shared libraries to ensure it can override the `malloc()` functions with its own versions. This can be achieved, for example, by setting the `LD_PRELOAD` environmental variable to point to `libsdrad.so` to instruct the Linux dynamic linker to preload `libsdrad.so` before `glibc`.

```
LD_PRELOAD=/path/to/secure-rewind-and-discard/src/libsdrad.so
``` 


### Hardware Requirements
SDRaD requires a CPU supporting Intel Memory Protection Keys (MPK) and a [Linux Kernel supporting MPK](https://www.kernel.org/doc/html/next/core-api/protection-keys.html)


## Simple Example
We provide several examples that demonstrate the library's use in [examples/](./examples/)


## License 
© Ericsson AB 2022-2023

BSD 3-Clause License

The modified TLSF implementation by mattconte/tlsf licensed under BSD 3-Clause License
