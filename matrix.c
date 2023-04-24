#include "matrix.h"

#include <linux/gpio/driver.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/gpio.h>

#include "characters.h"

#define COL_ONE 5
#define COL_TWO 6
#define COL_THREE 16
#define COL_FOUR 20
#define COL_FIVE 21

#define ROW_ONE 18
#define ROW_TWO 23
#define ROW_THREE 4
#define ROW_FOUR 24
#define ROW_FIVE 17
#define ROW_SIX 27
#define ROW_SEVEN 22

static int cols[] = {COL_ONE, COL_TWO, COL_THREE, COL_FOUR, COL_FIVE};
static int rows[] = {ROW_ONE,  ROW_TWO, ROW_THREE, ROW_FOUR,
                     ROW_FIVE, ROW_SIX, ROW_SEVEN};

// Stored as an array of rows that each hold an entire column
static char** matrix_buffer = NULL;

// The index of the first column of the image that is currently being displayed
static int matrix_buffer_location = 0;
// The length of the image currently in the buffer
static int matrix_buffer_length = 0;

int matrix_init(void) {
  for (int i = 0; i < COLS; i++) {
    // Check that the GPIO pins are valid
    if (!gpio_is_valid(cols[i])) {
      printk(KERN_INFO "Invalid GPIO: %d\n", cols[i]);
      return -ENODEV;
    }
    // Request the GPIO pins
    if (gpio_request(cols[i], "out")) {
      printk(KERN_INFO "Failed to request GPIO %d\n", cols[i]);
      return -ENODEV;
    }
    // Set the GPIO pins to output
    if (gpio_direction_output(cols[i], 0)) {
      printk(KERN_INFO "Failed to set GPIO direction %d\n", cols[i]);
      return -ENODEV;
    } else {
      gpio_set_value(cols[i], 0);
    }
  }
  for (int i = 0; i < ROWS; i++) {
    if (!gpio_is_valid(rows[i])) {
      printk(KERN_INFO "Invalid GPIO: %d\n", rows[i]);
      return -ENODEV;
    }
    if (gpio_request(rows[i], "out")) {
      printk(KERN_INFO "Failed to request GPIO %d\n", rows[i]);
      return -ENODEV;
    }
    if (gpio_direction_output(rows[i], 0)) {
      printk(KERN_INFO "Failed to set GPIO direction %d\n", rows[i]);
      return -ENODEV;
    } else {
      gpio_set_value(rows[i], 0);
    }
  }
  printk(KERN_INFO "GPIO initialized\n");

  // Framebuffer stored as an array of rows that each hold an entire column of the
  // image. Cols are lengthened when scrolling over characters of a string.
  // Zero allocated
  matrix_buffer = kzalloc(ROWS * sizeof(char*), GFP_KERNEL);
  for (int i = 0; i < ROWS; i++) {
    matrix_buffer[i] = kzalloc(COLS * sizeof(char), GFP_KERNEL);
  }
  matrix_buffer_length = COLS;
  return 0;
}

void free_matrix_buffer(void) {
  if (matrix_buffer != NULL) {
    for (int i = 0; i < ROWS; i++) {
      kfree(matrix_buffer[i]);
    }
    kfree(matrix_buffer);
    matrix_buffer = NULL;
  }
}

int matrix_free(void) {
  matrix_display_clear();
  for (int i = 0; i < COLS; i++) {
    gpio_free(cols[i]);
  }
  for (int i = 0; i < ROWS; i++) {
    gpio_free(rows[i]);
  }
  free_matrix_buffer();
  printk(KERN_INFO "GPIO cleaned up\n");
  return 0;
}

int matrix_check_col(int col) {
  if (col < 0 || col >= COLS) {
    printk(KERN_INFO "Invalid col: %d\n", col);
    return -1;
  }
  return 0;
}

int matrix_check_row(int row) {
  if (row < 0 || row >= ROWS) {
    printk(KERN_INFO "Invalid row: %d\n", row);
    return -1;
  }
  return 0;
}

int matrix_check_pixel(int row, int col) {
  if (matrix_check_row(row)) return -1;
  if (matrix_check_col(col)) return -1;
  return 0;
}

// sets the "framebuffer" to all 0s
void matrix_set_clear(void) {
  for (int i = 0; i < ROWS; i++) {
    memset(matrix_buffer[i], 0, matrix_buffer_length * sizeof(char));
  }
  matrix_buffer_location = 0;
  matrix_buffer_length = COLS;
}

void matrix_set_row(int row, int val) {
  if (matrix_check_row(row)) return;
  for (int i = 0; i < COLS; i++) {
    matrix_buffer[row][i] = val;
  }
  matrix_buffer_location = 0;
  matrix_buffer_length = COLS;
}

void matrix_set_col(int col, int val) {
  if (matrix_check_col(col)) return;
  for (int i = 0; i < ROWS; i++) {
    matrix_buffer[i][col] = val;
  }
  matrix_buffer_location = 0;
  matrix_buffer_length = COLS;
}

void matrix_set_pixel(int row, int col, int val) {
  if(matrix_check_pixel(row, col)) return;
  matrix_buffer[row][col] = val;
  matrix_buffer_location = 0;
  matrix_buffer_length = COLS;
}

void matrix_set_character(char c) {
  const char(*character_map)[ROWS][COLS] = character_get_array(c);
  for(int i = 0; i < ROWS; i++) {
    memcpy(matrix_buffer[i], (*character_map)[i], COLS);
  }
  matrix_buffer_location = 0;
  matrix_buffer_length = COLS;
}

void matrix_set_string(const char* str) {
  // copy the string so we can modify it
  char* str_copy = kmalloc(strlen(str) + 1, GFP_KERNEL);
  strcpy(str_copy, str);
  // remove any newlines or returns
  str_copy[strcspn(str_copy, "\r\n")] = 0;
  
  matrix_set_clear();

  // we need space for each character and a space between the characters, and a blank space at the beginning
  matrix_buffer_length = strlen(str_copy) * (COLS + 1) + COLS;

  for (int row = 0; row < ROWS; row++) {
    matrix_buffer[row] = krealloc(matrix_buffer[row], matrix_buffer_length + COLS, GFP_KERNEL);
  }

  // copy each character of str into the string buffer
  for (int i = 0; i < strlen(str_copy); i++) {
    const char(*character_map)[ROWS][COLS] = character_get_array(str_copy[i]);
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        // copy the character map into the string buffer, and leave 1 space
        // between characters and one screen black at the beggining
        matrix_buffer[row][COLS + i * (COLS + 1) + col] = (*character_map)[row][col];
      }
    }
  }
  // restart at the beggining
  matrix_buffer_location = 0;

  kfree(str_copy);
}

const char (*matrix_get_pixels(void))[ROWS][COLS] {
  return (const char(*)[ROWS][COLS])matrix_buffer;
}

int matrix_get_location(void) {
  return matrix_buffer_location;
}

// turns off all GPIO pins
void matrix_display_clear(void) {
  for (int i = 0; i < COLS; i++) {
    gpio_set_value(cols[i], 0);
  }
  for (int i = 0; i < ROWS; i++) {
    gpio_set_value(rows[i], 0);
  }
}

void matrix_display_row(int row) {
  if (matrix_check_row(row)) return;
  for (int i = 0; i < COLS; i++) {
    gpio_set_value(cols[i], matrix_buffer[row][i]);
  }
  for (int i = 0; i < ROWS; i++) {
    gpio_set_value(rows[i], i == row ? 0 : 1);
  }
}

void matrix_display_col(int col) {
  if (matrix_check_col(col)) return;
  if(matrix_buffer_location >= matrix_buffer_length) return;
  for (int i = 0; i < ROWS; i++) {
    gpio_set_value(rows[i], matrix_buffer[i][col + matrix_buffer_location]);
  }
  for (int i = 0; i < COLS; i++) {
    gpio_set_value(cols[i], i == col ? 0 : 1);
  }
}

void matrix_display_scroll(void) {
  if (matrix_buffer == NULL) return;
  if (matrix_buffer_location >= matrix_buffer_length)
    matrix_buffer_location = 0;
  matrix_buffer_location++;
}