
obj-m += syscalladder.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc test/test.c -o test/test.o

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

