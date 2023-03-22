CFLAGS_led-matrix-control.o := -std=gnu99

obj-m := led-matrix-control.o

clean :
	rm -f *.o *.ko *.cmd *.mod *.mod.c *.symvers *.order
	find . -name '*.cmd' -delete