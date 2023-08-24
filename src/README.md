
This folder contains the source code of the SDRaD Project.

### Compiler Flag

To support multithreading applications, run
```
SDRAD_CONFIG=-DSDRAD_MULTITHREAD make
```

To build with zeroing domain after rewinding  run 
```
SDRAD_CONFIG=-DZERO_DOMAIN make
```

### Changing SDRaD Domain Size
As default domain size numbers, the following values are used: 

* Execution Domain Default Heap Size `0x100000000` (4GB) 
* Execution Domain Default Stack Size `0x800000` (8MB) 
* Data Domain Default Heap Size `0x100000000` (4GB)  

These numbers can be customized by using the following environmental variables. Please customize them with decimal numbers:

```
export SDRAD_STACK_SIZE
export SDRAD_HEAP_SIZE 
export SDRAD_DATA_DOMAIN_SIZE
```





