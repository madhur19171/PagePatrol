obj-m += insert_file_vaddr_into_inactive_list.o

# Enable BTF generation
KBUILD_CFLAGS += -g -O2

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
