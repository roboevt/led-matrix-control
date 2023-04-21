#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stddef.h>

#include "matrix.h"
#include "timer.h"

static char character = 'A';
static int fps = 1;
static char *string = NULL;

// Indicates which rows are completly lit
static ssize_t rows_show(struct kobject *kobj, struct kobj_attribute *attr,
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

static ssize_t rows_store(struct kobject *kobj, struct kobj_attribute *attr,
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
static ssize_t col_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
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

static ssize_t col_store(struct kobject *kobj, struct kobj_attribute *attr,
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

static ssize_t character_show(struct kobject *kobj, struct kobj_attribute *attr,
                              char *buf) {
  return sprintf(buf, "%c\n", character);
}

static ssize_t character_store(struct kobject *kobj,
                               struct kobj_attribute *attr, const char *buf,
                               size_t count) {
  character = buf[0];
  matrix_set_character(character);
  return count;
}

static ssize_t fps_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
  return sprintf(buf, "%d\n", fps);
}

static ssize_t fps_store(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buf, size_t count) {
  int sec = 0;
  int nsec = 0;
  int ret = kstrtoint(buf, 10, &fps);
  if (ret < 0) return ret;
  if (fps) {
    nsec = 1000000000 / fps;
    sec = 0;
  } else {               // setting to 0 fps
    sec = __INT_MAX__;   // 63 years
    nsec = __INT_MAX__;  // 2 seconds
  }
  timer_set_frame_interval(sec, nsec);
  return count;
}

static ssize_t pixels_show(struct kobject *kobj, struct kobj_attribute *attr,
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

static ssize_t pixels_store(struct kobject *kobj, struct kobj_attribute *attr,
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

static ssize_t string_show(struct kobject *kobj, struct kobj_attribute *attr,
                           char *buf) {
  return sprintf(buf, "%s\n", string);
}

static ssize_t string_store(struct kobject *kobj, struct kobj_attribute *attr,
                            const char *buf, size_t count) {
  // Ensure that string can fit buf;
  if (!string) {
    if (!kmalloc(count, GFP_KERNEL)) return -ENOMEM;
  } else {
    if (!krealloc(string, count, GFP_KERNEL)) return -ENOMEM;
  }
  // not sure why the compiler requires this, but it doesn't hurt
  if (string) strcpy(string, buf);

  matrix_set_string(buf);
  return count;
}

// Attributes

static struct kobj_attribute rows_attribute =
    __ATTR(rows, 0664, rows_show, rows_store);

static struct kobj_attribute col_attribute =
    __ATTR(cols, 0664, col_show, col_store);

static struct kobj_attribute character_attribute =
    __ATTR(character, 0664, character_show, character_store);

static struct kobj_attribute fps_attribute =
    __ATTR(fps, 0664, fps_show, fps_store);

static struct kobj_attribute pixels_attribute =
    __ATTR(pixels, 0664, pixels_show, pixels_store);

static struct kobj_attribute string_attribute =
    __ATTR(string, 0664, string_show, string_store);

static struct attribute *attrs[] = {&rows_attribute.attr,
                                    &col_attribute.attr,
                                    &character_attribute.attr,
                                    &fps_attribute.attr,
                                    &pixels_attribute.attr,
                                    &string_attribute.attr,
                                    NULL};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *led_matrix;

static int __init led_module_init(void) {
  int retval;
  printk(KERN_INFO "LED Matrix Module loading\n");

  led_matrix = kobject_create_and_add("led-matrix", NULL);
  if (!led_matrix) return -ENOMEM;

  // Create the files associated with this kobject
  retval = sysfs_create_group(led_matrix, &attr_group);
  if (retval) {
    kobject_put(led_matrix);
    return retval;
  }
  printk(KERN_INFO "Kobject created\n");

  retval = matrix_init();
  if (retval) {
    kobject_put(led_matrix);
    return retval;
  }
  matrix_display_clear();
  matrix_set_character('A');

  timer_init();

  printk(KERN_INFO "LED Matrix Module loaded\n");
  return retval;
}

static void __exit led_module_exit(void) {
  kobject_put(led_matrix);
  printk(KERN_INFO "Kobject removed\n");
  matrix_free();
  timer_exit();
}

module_init(led_module_init);
module_exit(led_module_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Eric Todd");
