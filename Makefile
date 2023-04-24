CFLAGS_led-matrix-module.o := -std=gnu99 -Wall
CFLAGS_matrix.o := -std=gnu99 -Wall
CFLAGS_timer.o := -std=gnu99 -Wall
CFLAGS_characters.o := -std=gnu99 -Wall
CFLAGS_led-matrix-module-utils.o := -std=gnu99 -Wall

obj-m := led-matrix.o

led-matrix-objs := led-matrix-module.o matrix.o timer.o characters.o led-matrix-module-utils.o

clean :
	rm -f *.o *.ko *.cmd *.mod *.mod.c *.symvers *.order
	find . -name '*.cmd' -delete