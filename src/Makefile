OUTPUT_PREFIX ?= .
OUT_FILE_NAME = libsdrad.a libsdrad.so
CC=gcc
SDRAD_CONFIG+= -DPKEY_ENABLE  -DSDRAD_MULTITHREAD  #-DZERO_DOMAIN
DEPS = sdrad.h sdrad_heap_mng.h sdrad_cache.h sdrad_error_handling.h\
	   sdrad_pkey.h sdrad_signal.h sdrad_thread.h 

CFLAGS+= -pthread -Wl,--no-as-needed  -ldl -O2 \
		 -Wpointer-arith  -fstack-protector-strong  -fPIC \

CFLAGS+= -Wall -Wextra -Wformat=2 -Wconversion -Wsign-conversion\
		 -Wtrampolines 


LDFLAGS = -ldl

OBJECT_ROOT = sdrad.o  tlsf.o sdrad_heap_mng.o  sdrad_exit.o sdrad_enter.o \
			  sdrad_init.o sdrad_pkey.o sdrad_thread.o sdrad_signal.o \
			  sdrad_cache.o sdrad_error_handling.o sdrad_enter_root.o \
			  sdrad_exit_root.o sdrad_api.o

all: $(OBJECT_ROOT) $(OUT_FILE_NAME)

%.o: %.c $(DEPS)
	$(CC)  -g   $(CFLAGS) $(SDRAD_CONFIG) -c -o $@ $< 

%.o: %.S $(DEPS)
	$(CC)  -g   $(CFLAGS) $(SDRAD_CONFIG) -c -o $@ $< 


libsdrad.so: $(OBJECT_ROOT) 
	mkdir -p $(OUTPUT_PREFIX)
	$(CC) -shared  -fPIC  $(CFLAGS) -o $(OUTPUT_PREFIX)/libsdrad.so  $(OBJECT_ROOT) 
 
libsdrad.a: $(OBJECT_ROOT)
	mkdir -p $(OUTPUT_PREFIX)
	ar -rc $(OUTPUT_PREFIX)/libsdrad.a $(OBJECT_ROOT) 

clean:
	rm -f *.o  $(OUTPUT_PREFIX)/*.a $(OUTPUT_PREFIX)/*.so 