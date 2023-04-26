#include "matrix.h"
#include "timer.h"
#include "led-matrix-module.h"

// Link getters and setters to the kernel attributes

// Indicates which rows are completly lit
static struct kobj_attribute rows_attribute =
    __ATTR(rows, 0664, rows_show, rows_store);
// Indicates which columns are completly lit
static struct kobj_attribute col_attribute =
    __ATTR(cols, 0664, col_show, col_store);
// The character to display
static struct kobj_attribute character_attribute =
    __ATTR(character, 0664, character_show, character_store);
// The rate at which the entire screen updates (relevent when scrolling through a string)
static struct kobj_attribute fps_attribute =
    __ATTR(fps, 0664, fps_show, fps_store);
// Which pixels are illuminated
static struct kobj_attribute pixels_attribute =
    __ATTR(pixels, 0664, pixels_show, pixels_store);
// The string to display
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

// Initializes the module and matrix/timer
static int __init led_module_init(void) {
  int ret;
  printk(KERN_INFO "LED Matrix Module loading\n");

  led_matrix = kobject_create_and_add("led-matrix", NULL);
  if (!led_matrix) return -ENOMEM;

  // Create the files associated with this kobject
  ret = sysfs_create_group(led_matrix, &attr_group);
  if (ret) {
    kobject_put(led_matrix);
    return ret;
  }
  printk(KERN_INFO "Kobject created\n");

  ret = matrix_init();
  if (ret) {
    kobject_put(led_matrix);
    return ret;
  }
  matrix_display_clear();
  timer_init();

  matrix_set_character('A');

  printk(KERN_INFO "LED Matrix Module loaded\n");
  return ret;
}

static void __exit led_module_exit(void) {
  kobject_put(led_matrix);
  printk(KERN_INFO "Kobject removed\n");
  matrix_free();
  timer_exit();
  led_matrix_exit();
}

module_init(led_module_init);
module_exit(led_module_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Eric Todd");
