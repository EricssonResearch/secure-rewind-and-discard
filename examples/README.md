# SDRaD Examples and Test Cases
This folder contains examples of how to use the SDRaD library

## Prerequisites

Before compiling and running the examples, make sure you have built the SDRaD library (`libsdrad.so`) and added the SDRaD source directory containing the shared object to the Linux dynamic linker search path.

To compile SDRaD, Run `make` in [src/](../src/):

```
cd ../src
make
```

To add the SDRaD source directory containing the resulting shared object to the dynamic linker search path set the `LD_LIBRARY_PATH` environmental variable to include the [src/](../src/) directory:

```
export LD_LIBRARY_PATH='/path/to/secure-rewind-and-discard/src'
```

## Building and running the SDRaD examples

To build examples, run `make` in the [examples/](./examples/) directory:

```
cd ../examples
make
```

If `libsdrob.so` can be found in the dynamic linker search path, the resulting example executables can be run as indicated below:

* An example of how to use  `sdrad_call`. If the user enters more than 6 bytes, the stackoverflow can be detected. 
```
./sdrad_call_example
Enter Keyword 
hello
Current Buf : hello
Enter Keyword 
merhaba
ERROR_BAD_INPUT 
Enter Keyword:
```
* SDRaD allows rewinding applications in the case of stackoverflow in the isolated domain. The application should be compiled with `__wrap_stack_chk_fail`  `-fstack-protector` compiler options.  If the user enters more than 4 bytes, the stackoverflow can be detected. 
```
./stackoverflow_handler
WAITING REQUEST 
merhaba   
HANDLING REQUEST 
Success: Domain Violation Detected
WAITING REQUEST 
```
* SDRaD allows rewinding applications in case of a segmentation fault in the nested domain. 
```
./segfault_handler
Success: Domain Violation Detected
```

* The nested domain cannot modify global variables.
```
./global_data_test
Success: Domain Violation Detected
```

* The nested domain cannot modify the stack area of the parent domain.
```
./stack_violation
Success: Domain Violation Detected
```

* SDRaD protects the thread memory area from its own nested domain. 
```
./pthread_example
Success: Domain Violation Detected
```