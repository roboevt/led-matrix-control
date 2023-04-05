#define COLS 5
#define ROWS 7

// verify and initialize the GPIO pins
int matrix_init(void);
// turn off all GPIO pins and release them
int matrix_free(void);

// check if the column is valid
int matrix_check_col(int col);
// check if the row is valid
int matrix_check_row(int row);

// set the framebuffer to all zeros
void matrix_set_clear(void);
// set the value of one row
void matrix_set_row(int row, int val);
// set the value of one column
void matrix_set_col(int col, int val);
// set the value of one pixel
void matrix_set_pixel(int row, int col, int val);
// set the framebuffer to a representation of a character
void matrix_set_character(char c);

// turn off all GPIO pins
void matrix_display_clear(void);
// display one row of the framebuffer to the matrix
void matrix_display_row(int row);
// display one column of the framebuffer to the matrix
void matrix_display_col(int col);