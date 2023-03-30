#include "matrix.h"

#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>

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

static char matrix[ROWS][COLS] = {
    {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};

int matrix_init(void) {
  for (int i = 0; i < COLS; i++) {
    if (!gpio_is_valid(cols[i])) {
      printk(KERN_INFO "Invalid GPIO: %d\n", cols[i]);
      return -ENODEV;
    }
    if (gpio_request(cols[i], "out")) {
      printk(KERN_INFO "Failed to request GPIO %d\n", cols[i]);
      return -ENODEV;
    }
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
  return 0;
}

int matrix_free(void) {
  matrix_display_clear();
  for (int i = 0; i < COLS; i++) {
    gpio_free(cols[i]);
  }
  for (int i = 0; i < ROWS; i++) {
    gpio_free(rows[i]);
  }
  printk(KERN_INFO "GPIO cleaned up\n");
  return 0;
}

// char (*matrix_get(void))[COLS] { return matrix; }
// static int check_row_valid(int row) {
//   if (row < 0 || row >= ROWS) {
//     printk(KERN_INFO "Invalid row: %d\n", row);
//     return -1;
//   }
//   return 0;
// }

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

// sets the "framebuffer" to all 0s
void matrix_set_clear(void) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      matrix[i][j] = 0;
    }
  }
}

void matrix_set_row(int row, int val) {
  if (matrix_check_row(row)) return;
  for (int i = 0; i < COLS; i++) {
    matrix[row][i] = val;
  }
}

void matrix_set_col(int col, int val) {
  if (matrix_check_col(col)) return;
  for (int i = 0; i < ROWS; i++) {
    matrix[i][col] = val;
  }
}

void matrix_set_pixel(int row, int col, int val) {
  if (matrix_check_row(row)) return;
  if (matrix_check_col(col)) return;
  matrix[row][col] = val;
}

void matrix_set_character(char c) {
  const char (*character_map)[ROWS][COLS] = character_get_array(c);
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      matrix[i][j] = (*character_map)[i][j];
    }
  }
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
    gpio_set_value(cols[i], matrix[row][i]);
  }
  for (int i = 0; i < ROWS; i++) {
    gpio_set_value(rows[i], i == row ? 0 : 1);
  }
}

void matrix_display_col(int col) {
  if (matrix_check_col(col)) return;
  for (int i = 0; i < ROWS; i++) {
    gpio_set_value(rows[i], matrix[i][col]);
  }
  for (int i = 0; i < COLS; i++) {
    gpio_set_value(cols[i], i == col ? 0 : 1);
  }
}
