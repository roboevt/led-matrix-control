#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <stdbool.h>

#include "led-matrix-module.h"
#include "matrix.h"
#include "timer.h"

static char character = 'A';
static int fps = 0;
static int scrollingFps = DEFAULT_SCROLL_FPS;
static char *string = NULL;

ssize_t rows_show(struct kobject *kobj, struct kobj_attribute *attr,
                  char *buf) {
  const char **currentFramebuffer = matrix_get_pixels();
  char *originalStart = buf;
  int ret;
  for (int row = 0; row < ROWS; row++) {
    bool entireRowLit = true;
    for (int col = 0; col < COLS; col++) {
      if (!(currentFramebuffer)[row][col]) {
        entireRowLit = false;
        break;
      }
    }
    if (entireRowLit) {
      ret = sprintf(buf, "%d ", row + 1);
      if (ret < 0) return ret;
      buf += ret;
    }
  }
  ret = sprintf(buf, "\n");
  if (ret < 0) return ret;
  buf += ret;
  return buf - originalStart;
}

ssize_t rows_store(struct kobject *kobj, struct kobj_attribute *attr,
                   const char *buf, size_t count) {
  int bufIndex = 0;
  int row, charsRead;

  while (buf[bufIndex] != '\0') {
    if (sscanf(buf + bufIndex, "%d%n", &row, &charsRead) == 1) {
      // we have read a row number
      if (row == 0) {
        // clear the matrix
        matrix_set_clear();
        return count;
      }
      if (matrix_check_row(row - 1) && matrix_check_row((-row) - 1)) {
        // requested row is invalid
        return -EINVAL;
      }
      // requested row is valid
      if (matrix_check_row(row - 1)) {
        // requested row is negative, so clear row
        matrix_set_row((-row) - 1, 0);
      } else {
        // requested row is positive, so set row
        matrix_set_row(row - 1, 1);
      }
    } else {
      // read failed
      return -EINVAL;
    }
    bufIndex += charsRead;  // skip over the characters we just read
    while (isspace(buf[bufIndex])) bufIndex++;  // skip over whitespace
  }
  fps = 0;
  return count;
}

// Indicates which columns are completly lit
ssize_t col_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  const char **currentFramebuffer = matrix_get_pixels();
  char *originalStart = buf;
  int ret;
  for (int col = 0; col < COLS; col++) {
    bool entireColLit = true;
    for (int row = 0; row < ROWS; row++) {
      printk(KERN_INFO "row %d, col %d, val %d\n", row, col,
             (currentFramebuffer)[row][col]);
      if (!(currentFramebuffer)[row][col]) {
        entireColLit = false;
        break;
      }
    }
    if (entireColLit) {
      printk(KERN_INFO "col %d is lit\n", col + 1);
      ret = sprintf(buf, "%d ", col + 1);
      if (ret < 0) return ret;
      buf += ret;
    }
  }
  ret = sprintf(buf, "\n");
  if (ret < 0) return ret;
  buf += ret;
  return buf - originalStart;
}

ssize_t col_store(struct kobject *kobj, struct kobj_attribute *attr,
                  const char *buf, size_t count) {
  int bufIndex = 0;
  int col, charsRead;

  while (buf[bufIndex] != '\0') {
    if (sscanf(buf + bufIndex, "%d%n", &col, &charsRead) == 1) {
      // we have read a col number
      if (col == 0) {
        // clear the matrix
        matrix_set_clear();
        return count;
      }
      if (matrix_check_col(col - 1) && matrix_check_col((-col) - 1)) {
        // requested col is invalid
        return -EINVAL;
      }
      // requested col is valid
      if (matrix_check_col(col - 1)) {
        // requested col is negative, so clear col
        matrix_set_col((-col) - 1, 0);
      } else {
        // requested col is positive, so set col
        matrix_set_col(col - 1, 1);
      }
    } else {
      // read failed
      return -EINVAL;
    }
    bufIndex += charsRead;  // skip over the characters we just read
    while (isspace(buf[bufIndex])) bufIndex++;  // skip over whitespace
  }
  fps = 0;
  return count;
}

ssize_t character_show(struct kobject *kobj, struct kobj_attribute *attr,
                       char *buf) {
  return sprintf(buf, "%c\n", character);
}

ssize_t character_store(struct kobject *kobj, struct kobj_attribute *attr,
                        const char *buf, size_t count) {
  character = buf[0];
  matrix_set_character(character);
  fps = 0;
  return count;
}

ssize_t fps_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  return sprintf(buf, "%d\n", fps);
}
// converts fps to period and updates the timer
static void set_fps(void) {
  int sec = 0;
  int nsec = 0;
  if (fps) {
    // period -> frequency, 1 billion nanoseconds in a second
    nsec = 1000000000 / fps;
    sec = 0;
    // remember the fps for scrolling only when not setting fps to 0
    scrollingFps = fps;
  } else {               // setting to 0 fps
    sec = __INT_MAX__;   // 63 years
    nsec = __INT_MAX__;  // 2 seconds
  }
  timer_set_frame_interval(sec, nsec);
}

ssize_t fps_store(struct kobject *kobj, struct kobj_attribute *attr,
                  const char *buf, size_t count) {
  int ret = kstrtoint(buf, 10, &fps);
  if (ret < 0) return ret;

  set_fps();
  return count;
}

ssize_t pixels_show(struct kobject *kobj, struct kobj_attribute *attr,
                    char *buf) {
  const char **currentFramebuffer = matrix_get_pixels();
  char *originalStart = buf;  // for calculating length at the the end
  int ret;

  for (int row = 0; row < ROWS; row++) {
    for (int col = 0; col < COLS; col++) {
      if ((currentFramebuffer)[row][col]) {
        int ret = sprintf(buf, "%d,%d ", row + 1, col + 1);
        if (ret < 0) return ret;
        buf += ret;
      }
    }
  }
  ret = sprintf(buf, "\n");
  if (ret < 0) return ret;
  buf += ret;
  return buf - originalStart;
}

ssize_t pixels_store(struct kobject *kobj, struct kobj_attribute *attr,
                     const char *buf, size_t count) {
  int i = 0;
  int row, col, charsRead;
  while (buf[i] != '\0') {
    if (sscanf(buf + i, "%d,%d%n", &col, &row, &charsRead) == 2) {
      // we have read a row and col number
      if (row == 0 && col == 0) {
        // clear the matrix
        matrix_set_clear();
        return count;
      }
      if (matrix_check_pixel(row - 1, col - 1) &&
          matrix_check_pixel(row - 1, (-col) - 1)) {
        // requested pixel is invalid
        return -EINVAL;
      }
      // requested pixel is valid
      if (matrix_check_pixel(row - 1, col - 1)) {
        // requested pixel is negative, so clear pixel
        matrix_set_pixel(row - 1, (-col) - 1, 0);
      } else {
        // requested pixel is positive, so set pixel
        matrix_set_pixel(row - 1, col - 1, 1);
      }
      matrix_set_pixel(row - 1, col - 1, 1);
    } else {
      return -EINVAL;
    }
    i += charsRead;               // skip over the characters we just read
    while (isspace(buf[i])) i++;  // skip over whitespace
  }
  fps = 0;
  return count;
}

ssize_t string_show(struct kobject *kobj, struct kobj_attribute *attr,
                    char *buf) {
  return sprintf(buf, "%s\n", string);
}

ssize_t string_store(struct kobject *kobj, struct kobj_attribute *attr,
                     const char *buf, size_t count) {
  // Ensure that string can fit buf;
  if (!string) {
    if (!kmalloc(count, GFP_KERNEL)) return -ENOMEM;
  } else {
    if (!krealloc(string, count, GFP_KERNEL)) return -ENOMEM;
  }
  // not sure why the compiler requires the null check, but it doesn't hurt
  if (string) strcpy(string, buf);

  matrix_set_string(buf);

  // If fps is currently 0, reset it to the last selected value.
  if (!fps) {
    fps = scrollingFps;
    set_fps();
  }
  return count;
}

void led_matrix_exit(void) { kfree(string); }