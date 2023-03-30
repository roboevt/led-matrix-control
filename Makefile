CFLAGS_led-matrix-module.o := -std=gnu99
CFLAGS_matrix.o := -std=gnu99
CFLAGS_timer.o := -std=gnu99
CFLAGS_characters.o := -std=gnu99

obj-m := led-matrix.o

led-matrix-objs := led-matrix-module.o matrix.o timer.o characters.o

clean :
	rm -f *.o *.ko *.cmd *.mod *.mod.c *.symvers *.order
	find . -name '*.cmd' -delete