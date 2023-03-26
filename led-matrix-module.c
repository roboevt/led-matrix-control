#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <stddef.h>

#include "matrix.h"
#include "timer.h"

static int row = 1;
static int col = 1;
static int sec = 0;
static int nsec = 500000000;

static ssize_t row_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
  return sprintf(buf, "%d\n", row);
}

static ssize_t row_store(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buf, size_t count) {
  int ret = kstrtoint(buf, 10, &row);
  if (ret < 0) return ret;
  if (row < 8 && row > 0) {
    matrix_set_row(row - 1, 1);
  } else {
    matrix_set_clear();
  }
  return count;
}

static ssize_t col_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
  return sprintf(buf, "%d\n", col);
}

static ssize_t col_store(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buf, size_t count) {
  int ret = kstrtoint(buf, 10, &col);
  if (ret < 0) return ret;
  if (col < 6 && col > 0) {
    matrix_set_col(col - 1, 1);
  } else {
    matrix_set_clear();
  }
  return count;
}

static ssize_t sec_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
  return sprintf(buf, "%d\n", sec);
}

static ssize_t sec_store(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buf, size_t count) {
  int ret = kstrtoint(buf, 10, &sec);
  if (ret < 0) return ret;
  timer_set_interval(sec, nsec);
  return count;
}

static ssize_t nsec_show(struct kobject *kobj, struct kobj_attribute *attr,
                         char *buf) {
  return sprintf(buf, "%d\n", nsec);
}

static ssize_t nsec_store(struct kobject *kobj, struct kobj_attribute *attr,
                          const char *buf, size_t count) {
  int ret = kstrtoint(buf, 10, &nsec);
  if (ret < 0) return ret;
  timer_set_interval(sec, nsec);
  return count;
}

static struct kobj_attribute row_attribute =
    __ATTR(row, 0664, row_show, row_store);

static struct kobj_attribute col_attribute =
    __ATTR(col, 0664, col_show, col_store);

static struct kobj_attribute sec_attribute =
    __ATTR(sec, 0664, sec_show, sec_store);

static struct kobj_attribute nsec_attribute =
    __ATTR(nsec, 0664, nsec_show, nsec_store);

/*
 * More complex function where we determine which variable is being accessed by
 * looking at the attribute for the "baz" and "bar" files.

static ssize_t b_show(struct kobject *kobj, struct kobj_attribute *attr,
                      char *buf) {
  int var;

  if (strcmp(attr->attr.name, "baz") == 0)
    var = baz;
  else
    var = bar;
  return sprintf(buf, "%d\n", var);
}

static ssize_t b_store(struct kobject *kobj, struct kobj_attribute *attr,
                       const char *buf, size_t count) {
  int var, ret;

  ret = kstrtoint(buf, 10, &var);
  if (ret < 0) return ret;

  if (strcmp(attr->attr.name, "baz") == 0)
    baz = var;
  else
    bar = var;
  return count;
}

static struct kobj_attribute baz_attribute = __ATTR(baz, 0664, b_show, b_store);
static struct kobj_attribute bar_attribute = __ATTR(bar, 0664, b_show, b_store);
*/

static struct attribute *attrs[] = {
    // &foo_attribute.attr, &baz_attribute.attr, &bar_attribute.attr,
    &row_attribute.attr, &col_attribute.attr, &sec_attribute.attr,
    &nsec_attribute.attr, NULL};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *led_matrix;

static int __init led_module_init(void) {
  int retval;
  printk(KERN_INFO "LED Matrix Module loading\n");

  led_matrix = kobject_create_and_add("led-matrix", NULL);
  if (!led_matrix) return -ENOMEM;

  /* Create the files associated with this kobject */
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
  matrix_set_row(1, 1);

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
