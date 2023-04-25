#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "led-matrix-module.h"
#include "matrix.h"
#include "timer.h"

static char character = 'A';
static int fps = 0;
static char *string = NULL;

ssize_t rows_show(struct kobject *kobj, struct kobj_attribute *attr,
                  char *buf) {
  const char(*current_framebuffer)[ROWS][COLS] = matrix_get_pixels();
  char *originalStart = buf;
  int ret;
  for (int row = 0; row < ROWS; row++) {
    bool entireRowLit = true;
    for (int col = 0; col < COLS; col++) {
      if (!(*current_framebuffer)[row][col]) {
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
  int row, chars_read;
  while (buf[bufIndex] != '\0') {
    if (sscanf(buf + bufIndex, "%d%n", &row, &chars_read) == 1) {
      if (matrix_check_row(row - 1)) return -EINVAL;
      matrix_set_row(row - 1, 1);
    } else {
      return -EINVAL;
    }
    bufIndex += chars_read;  // skip over the characters we just read
    while (isspace(buf[bufIndex])) bufIndex++;  // skip over whitespace
  }
  return count;
}

// Indicates which columns are completly lit
ssize_t col_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  const char(*current_framebuffer)[ROWS][COLS] = matrix_get_pixels();
  char *originalStart = buf;
  int ret;
  for (int col = 0; col < COLS; col++) {
    bool entireColLit = true;
    for (int row = 0; row < ROWS; row++) {
      if (!(*current_framebuffer)[row][col]) {
        entireColLit = false;
        break;
      }
    }
    if (entireColLit) {
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
  int col, chars_read;
  while (buf[bufIndex] != '\0') {
    if (sscanf(buf + bufIndex, "%d%n", &col, &chars_read) == 1) {
      if (matrix_check_col(col - 1)) return -EINVAL;
      matrix_set_col(col - 1, 1);
    } else {
      return -EINVAL;
    }
    bufIndex += chars_read;  // skip over the characters we just read
    while (isspace(buf[bufIndex])) bufIndex++;  // skip over whitespace
  }
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
  return count;
}

ssize_t fps_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  return sprintf(buf, "%d\n", fps);
}

static void set_fps(void) {
  int sec = 0;
  int nsec = 0;
  if (fps) {
    nsec = 1000000000 / fps;
    sec = 0;
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
  const char(*current_framebuffer)[ROWS][COLS] = matrix_get_pixels();
  char *originalStart = buf;  // for calculating length at the the end
  int ret;

  for (int row = 0; row < ROWS; row++) {
    for (int col = 0; col < COLS; col++) {
      if ((*current_framebuffer)[row][col]) {
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
  int row, col, chars_read;
  while (buf[i] != '\0') {
    if (sscanf(buf + i, "%d,%d%n", &col, &row, &chars_read) == 2) {
      matrix_set_pixel(row - 1, col - 1, 1);
    } else {
      return -EINVAL;
    }
    i += chars_read;              // skip over the characters we just read
    while (isspace(buf[i])) i++;  // skip over whitespace
  }
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

  // If fps is currently 0, set it to a default so the screen scrolls.
  if (!fps) {
    fps = DEFAULT_SCROLL_FPS;
    set_fps();
  }
  return count;
}

void led_matrix_exit(void) {
    kfree(string);
}