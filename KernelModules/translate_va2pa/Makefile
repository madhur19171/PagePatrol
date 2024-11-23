obj-m += translate_va2pa.o

# Enable BTF generation
KBUILD_CFLAGS += -g -O2

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
