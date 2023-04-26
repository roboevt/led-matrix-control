A kernel module to control a 5x7 matrix display.

Installation:
    Hardware: Connect the pins of the matrix display to the GPIO pins of the raspberry pi. According to the datasheet
    (https://datasheet.octopart.com/LTP-757G-Lite-On-datasheet-13707286.pdf) update the defines at the top of the matrix.c
    file to represent how the GPIO pins map to the matrix pins.

    Software: Set the LINUX_SOURCE environment variable to the location of your linux kernel source files. Issue this
    command in order to compile the module: 
    make -C $LINUX_SOURCE ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$PWD modules

    Copy the resulting led-matrix.ko file to your raspberry pi and install it with sudo insmod led-matrix.ko.
    For more information see this studio:
    (https://classes.engineering.wustl.edu/cse422/studios/04_modules.html)

    Build files can be cleaned up with the command: make clean

Check out the /sys/led-matrix folder for the interface to the module.
    rows/cols - A list (seperated by whitespace) of the fully illuminated rows or columns. Write new values to update.
        Negative values turn off the specific line.
    pixels - A list (seperated by whitespace) of the currently lit pixels. Write as coordinate pairs (x,y x2,y2, etc.)
        Negative values turn off the specified pixel.
    character - writing a character (ascii [48-122]) will display that character to the matrix.
    fps - This attribute controls the number of new frames per second when scrolling through a string.
        Will be set to 0 when a row, col, pixel, or character is set, and return to previous value with a new string.
    string - A string to scroll through on the display.
    
Explanation of components (see header files as well):
    led-matrix-module - Main code for actual kernel object. Initializes and registers sysfs attributes. It also
        initializes the matrix and timer code, and cleans everything up when the module is unloaded.
    
    led-matrix-module-util - Code for how the attributs should be stored and loaded. Interface from attribute to 
        module code.
        rows_shows returns a string containing a list of all the rows that are currently entirely lit.
        rows_store takes a string of whitespace seperated rows and sets them to be lit
        cols_* are the same as the rows functions, but for the columns of the matrix
        character_show returns the most recently displayed character.
        character_store sets the character to be displayed. The " " character clears the display.
        fps_show returns the current framerate.
        fps_store calculates the period appropriate for a desired framerate and sets the timer delay accordingly.
        pixels_show returns a list of currently lit pixels, as coordinate pairs seperated by spaces.
        pixels_store sets the pixels that should be lit, read as coordinate pairs seperated by whitespace.
        string_show returns the most recently requested string
        string_store sets a string that should be scrolled on the display.

    matrix - Code that directly controls the gpio pins. Exposes a simpler interface for writing information to the
        display as opposed to the gpio pins directly.
        matrix_init and matrix_free initialize and shutdown the gpio pins and memory, respectively. 
        The matrix_check_* functions check that specified locations are valid before illegally accessing them. 
        The matrix_set_* functions modify the framebuffer in some way. These changes will be shown during:
        The matrtrix_display_* functions. These modify the gpio pins' state to reflect the current state of the internal
        framebuffer (char** matrix_buffer). 

    timer - Code that deals with two timers, the scanline timer and the frame timer. The scanline timer runs very often,
        and scans across the columns of the matrix, in order to allow arbitrary patterns to be displayed. The frame timer
        runs when an animation is occuring - namely scrolling text in order to display a string of characters.
        timer_init and timer_exit initialize and start, or cancel and end, respectivly the two timers.
        the timer_set_* functions modify the delay each timer uses. Currently only timer_set_frame_interval is used.
    
    characters - A set of character maps. Ascii characters are stored as static const two dimensional arrays of pre-computed
        values. There is also a lookup table in the form of a switch statement that returns the array for a specified character,
        or a completly filled in square when an unkown char is used. Letters, numbers, upper and lowercase letters, and some
        symbols are mapped, but some ascii values are not.