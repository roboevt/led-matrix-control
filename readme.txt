A kernel module to control a 5x7 matrix display.

Installation:
    Hardware: Connect the pins of the matrix display to the GPIO pins of the raspberry pi. According to the datasheet
    (https://datasheet.octopart.com/LTP-757G-Lite-On-datasheet-13707286.pdf) update the defines at the top of the matrix.c
    file to represent how the GPIO pins map to the matrix pins.

    Software: Set the LINUX_SOURCE environment variable to the location of your linux kernel source files. Issue this
    command in order to compile the module: 
    make -C $LINUX_SOURCE ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$PWD modules

    Copy the resulting led-matrix.ko file to your raspberry pi and install it with sudo insmod led-matrix.ko.
    For more information check out this studio:
    (https://classes.engineering.wustl.edu/cse422/studios/04_modules.html)

Check out the /sys/led-matrix folder for the interface to the module.
    row/col - writing a number [1-7] or [1-5] respectively will illuminate that row/column. Out of range value will
        clear the display.
    character - writing a character (ascii [48-122]) will display that character to the matrix.
    fps - By default the module runs through a demonstration of the possible characters. This attribute controls the
        number of new frames per second. Disable by setting it to 0.