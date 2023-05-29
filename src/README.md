
This folder contains the source code of the SDRaD Project.

### Compiler Flag

* `-DSDRAD_MULTITHREAD` -> It enables supporting multithread application  
* `-DZERO_DOMAIN` -> It allows zeroing domain after rewinding 

### Changing SDRaD Domain Size
As default domain size numbers, the following values are used: 

* Execution Domain Default Heap Size `0x100000000` (4GB) 
* Execution Domain Default Stack Size `0x800000` (8MB) 
* Data Domain Default Heap Size `0x100000000` (4GB)  

These numbers can be customized by using the following environmental variables.

```
export SDRAD_STACK_SIZE
export SDRAD_HEAP_SIZE 
export SDRAD_DATA_DOMAIN_SIZE
```





