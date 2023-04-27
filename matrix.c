#include "matrix.h"

#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <stdbool.h>

#include "characters.h"

// GPIO pin numbers
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

// Stored as an array of rows that each hold an entire column. Cols grow when
// scrolling through text.
static char** matrixBuffer = NULL;

// The index of the first column of the image that is currently being displayed
static int matrixBufferLocation = 0;
// The length of the image currently in the buffer
static int matrixBufferLength = 0;
// current scrolling state
static bool isMatrixScrolling = false;

static int gpio_init(int pin) {
  // Check that the GPIO pins are valid
  if (!gpio_is_valid(pin)) {
    printk(KERN_INFO "Invalid GPIO: %d\n", pin);
    return -ENODEV;
  }
  // Request the GPIO pins
  if (gpio_request(pin, "out")) {
    printk(KERN_INFO "Failed to request GPIO %d\n", pin);
    return -ENODEV;
  }
  // Set the GPIO pins to output
  if (gpio_direction_output(pin, 0)) {
    printk(KERN_INFO "Failed to set GPIO direction %d\n", pin);
    return -ENODEV;
  } else {  // initialize pins to off
    gpio_set_value(pin, 0);
  }
  return 0;
}

int matrix_init(void) {
  int ret;
  for (int i = 0; i < COLS; i++) {
    ret = gpio_init(cols[i]);
    if (ret) return ret;
  }
  for (int i = 0; i < ROWS; i++) {
    ret = gpio_init(rows[i]);
    if (ret) return ret;
  }
  printk(KERN_INFO "GPIO initialized\n");

  // Framebuffer stored as an array of rows that each hold an entire column of
  // the image. Cols are lengthened when scrolling over characters of a string.
  // Zero allocated
  matrixBuffer = kzalloc(ROWS * sizeof(char*), GFP_KERNEL);
  for (int i = 0; i < ROWS; i++) {
    matrixBuffer[i] = kzalloc(COLS * sizeof(char), GFP_KERNEL);
  }
  matrixBufferLength = COLS;
  return 0;
}

void free_matrix_buffer(void) {
  if (matrixBuffer != NULL) {
    for (int i = 0; i < ROWS; i++) {
      kfree(matrixBuffer[i]);
    }
    kfree(matrixBuffer);
    matrixBuffer = NULL;
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
    memset(matrixBuffer[i], 0, matrixBufferLength * sizeof(char));
  }
  matrixBufferLocation = 0;
  matrixBufferLength = COLS;
  isMatrixScrolling = false;
}

static void disable_scrolling(void) {
  matrixBufferLocation = 0;
  matrixBufferLength = COLS;
  isMatrixScrolling = false;
}

void matrix_set_row(int row, int val) {
  if (matrix_check_row(row)) return;
  for (int i = 0; i < COLS; i++) {
    matrixBuffer[row][i] = val;
  }
  disable_scrolling();
}

void matrix_set_col(int col, int val) {
  if (matrix_check_col(col)) return;
  for (int i = 0; i < ROWS; i++) {
    matrixBuffer[i][col] = val;
  }
  disable_scrolling();
}

void matrix_set_pixel(int row, int col, int val) {
  if (matrix_check_pixel(row, col)) return;
  matrixBuffer[row][col] = val;
  disable_scrolling();
}

void matrix_set_character(char c) {
  const char(*characterMap)[ROWS][COLS] = character_get_array(c);
  for (int i = 0; i < ROWS; i++) {
    memcpy(matrixBuffer[i], (*characterMap)[i], COLS);
  }
  disable_scrolling();
}

void matrix_set_string(const char* str) {
  // copy the string so we can modify it
  char* strCopy = kmalloc(strlen(str) + 1, GFP_KERNEL);
  strcpy(strCopy, str);
  // remove any newlines or returns
  strCopy[strcspn(strCopy, "\r\n")] = 0;

  matrix_set_clear();

  // we need space for each character and a space between the characters, and a
  // blank space at the beginning
  matrixBufferLength = strlen(strCopy) * (COLS + 1) + COLS;

  for (int row = 0; row < ROWS; row++) {
    matrixBuffer[row] =
        krealloc(matrixBuffer[row], matrixBufferLength + COLS, GFP_KERNEL);
  }

  // copy each character of str into the string buffer
  for (int i = 0; i < strlen(strCopy); i++) {
    const char(*characterMap)[ROWS][COLS] = character_get_array(strCopy[i]);
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        // copy the character map into the string buffer, and leave 1 space
        // between characters and one screen black at the beggining
        matrixBuffer[row][COLS + i * (COLS + 1) + col] =
            (*characterMap)[row][col];
      }
    }
  }
  // restart at the beggining
  matrixBufferLocation = 0;
  isMatrixScrolling = true;
  kfree(strCopy);
}

const char** matrix_get_pixels(void) { return (const char**)matrixBuffer; }

int matrix_get_location(void) { return matrixBufferLocation; }

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
    gpio_set_value(cols[i], matrixBuffer[row][i]);
  }
  for (int i = 0; i < ROWS; i++) {
    gpio_set_value(rows[i], i == row ? 0 : 1);
  }
}

// display one column of the framebuffer to the matrix
// This is what is currently used by the timer
void matrix_display_col(int col) {
  if (matrix_check_col(col)) return;
  if (matrixBufferLocation >= matrixBufferLength) return;
  for (int i = 0; i < ROWS; i++) {
    // set the value of the row to the value of the pixel in the framebuffer
    gpio_set_value(rows[i], matrixBuffer[i][col + matrixBufferLocation]);
  }
  for (int i = 0; i < COLS; i++) {
    // and only turn on the column that we are currently displaying
    gpio_set_value(cols[i], i == col ? 0 : 1);
  }
}

void matrix_display_scroll(void) {
  if (!isMatrixScrolling) return;
  if (matrixBuffer == NULL) return;
  if (matrixBufferLocation >= matrixBufferLength) matrixBufferLocation = 0;
  matrixBufferLocation++;
}