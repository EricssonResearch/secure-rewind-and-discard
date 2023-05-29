CFLAGS=-I.
DEPS = ../src/sdrad_api.h


all:  segfault_handler stackoverflow_handler sdrad_call_example global_data_test stack_violation pthread_example

# Segfault example 
segfault_handler.o: segfault_handler.c 
	gcc -fstack-protector-strong -Wall -c segfault_handler.c   -g -o segfault_handler.o 

segfault_handler: segfault_handler.o  
	gcc -fstack-protector-strong -Wall segfault_handler.o  -L../src -lsdrad   -g -o segfault_handler


# Stack Overflow Example 
stackoverflow_handler.o: stackoverflow_handler.c $(DEPS)
	gcc -fstack-protector-strong -Wall -c stackoverflow_handler.c   -g -o stackoverflow_handler.o 

stackoverflow_handler: stackoverflow_handler.o  
	gcc -fstack-protector-strong -Wall stackoverflow_handler.o  -L../src -lsdrad -Wl,--wrap=__stack_chk_fail -g -o stackoverflow_handler


# sdrad_call()  Example 
sdrad_call_example.o: sdrad_call_example.c $(DEPS)
	gcc -fstack-protector-strong -Wall -c sdrad_call_example.c   -g -o sdrad_call_example.o 

sdrad_call_example: sdrad_call_example.o  
	gcc -fstack-protector-strong  -Wall sdrad_call_example.o -L../src -lsdrad -Wl,--wrap=__stack_chk_fail -g -o sdrad_call_example


# Global Data Test 
global_data_test.o: global_data_test.c $(DEPS)
	gcc -fstack-protector-strong -Wall -c global_data_test.c   -g -o global_data_test.o 

global_data_test: global_data_test.o  
	gcc -fstack-protector-strong -Wall  global_data_test.o  -L../src -lsdrad  -Wl,--wrap=__stack_chk_fail -g -o global_data_test

# Stack Violation Test  
stack_violation.o: stack_violation.c $(DEPS)
	gcc -fstack-protector-strong -Wall -c stack_violation.c   -g -o stack_violation.o 

stack_violation: stack_violation.o  
	gcc -fstack-protector-strong  -Wall stack_violation.o  -L../src -lsdrad  -Wl,--wrap=__stack_chk_fail -g -o stack_violation


# Thread Example Test  
pthread_example.o: pthread_example.c $(DEPS)
	gcc -fstack-protector-strong -Wall -c pthread_example.c   -g -o pthread_example.o 

pthread_example: pthread_example.o  
	gcc -fstack-protector-strong -Wall pthread_example.o  -L../src -lsdrad  -lpthread -Wl,--wrap=__stack_chk_fail -g -o pthread_example




clean:
	rm -f  *.o *.a main