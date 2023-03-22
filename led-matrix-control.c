// SPDX-License-Identifier: GPL-2.0
/*
 * Sample kobject implementation
 *
 * Copyright (C) 2004-2007 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2007 Novell Inc.
 */
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>

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
static int rows[] = {ROW_ONE, ROW_TWO, ROW_THREE, ROW_FOUR, ROW_FIVE, ROW_SIX, ROW_SEVEN};

static ktime_t timer_interval;
static struct hrtimer timer;
struct task_struct* kernelThread = NULL;

static enum hrtimer_restart restart_timer(struct hrtimer* timer) {
  wake_up_process(kernelThread);  // Wake up thread function to begin another loop.
  hrtimer_forward_now(
      timer,
      timer_interval);  // Ignoring return value since it's just the number of
                        // overruns and nothing useful can be done with it.
  return HRTIMER_RESTART;
}

static int threadFunction(void* data) {
  int i = 0;
  while(1) {
    i++;
	if(i > 7){
		i = 0;
		for(int j = 0; j < 7; j++){
			gpio_set_value(rows[j], 0);
		}
	} else {
		gpio_set_value(rows[i-1], 1);
	}
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
    if(kthread_should_stop()) {
      break;
    }
  }
  printk(KERN_INFO "threadFunction terminating.\n");
  return 0;
}

static int repeatingTimer_init(void) {
  printk(KERN_INFO "Repeating Timer module is loaded\n");

  //Start thread
  kernelThread = kthread_run(threadFunction, NULL, "threadFunction");
  if(!kernelThread) {
     printk(KERN_ALERT "Thread failed to create!\n");
     return EINTR;
  }

  //Initialize and start timer
  timer_interval = ktime_set(0, 100000000);
  printk(KERN_INFO "Timer initial timer value is %lldms \n",
         ktime_to_ms(timer_interval));
  hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  timer.function = restart_timer;
  hrtimer_start(&timer, timer_interval, HRTIMER_MODE_REL);
  return 0;
}

static void repeatingTimer_exit(void) {
  printk(KERN_INFO "Unloading module and attempting to cancel timer and function!\n");
  kthread_stop(kernelThread);

  if (hrtimer_cancel(&timer)) {
    printk(KERN_INFO "HRTIMER was active when cancel was called \n");
  } else {
    printk(KERN_INFO "HRTIMER was inactive when cancel was called \n");
  }
}

/*
 * This module shows how to create a simple subdirectory in sysfs called
 * /sys/kernel/kobject-example  In that directory, 3 files are created:
 * "foo", "baz", and "bar".  If an integer is written to these files, it can be
 * later read out of it.
 */

static int foo;
static int baz;
static int bar;

/*
 * The "foo" file where a static variable is read from and written to.
 */
static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr,
                        char *buf) {
  return sprintf(buf, "%d\n", foo);
}

static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr,
                         const char *buf, size_t count) {
  int ret;

  ret = kstrtoint(buf, 10, &foo);
  if (ret < 0) return ret;
  if(foo < 8 && foo > 0) {
	gpio_set_value(rows[foo-1], 1);
  } else {
	for(int i = 0; i < 7; i++) {
	  gpio_set_value(rows[i], 0);
	}
  }
  return count;
}

/* Sysfs attributes cannot be world-writable. */
static struct kobj_attribute foo_attribute =
    __ATTR(foo, 0664, foo_show, foo_store);

/*
 * More complex function where we determine which variable is being accessed by
 * looking at the attribute for the "baz" and "bar" files.
 */
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

/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *attrs[] = {
    &foo_attribute.attr, &baz_attribute.attr, &bar_attribute.attr,
    NULL, /* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory.  If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
static struct attribute_group attr_group = {
    .attrs = attrs,
	.name="led-matrix"
};

static struct kobject *led_matrix;

static int initialize_gpio(void) {
  for(int i = 0; i < 5; i++) {
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

  for(int i = 0; i < 7; i++) {
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

static void cleanup_gpio(void) {
  for(int i = 0; i < 5; i++) {
	gpio_set_value(cols[i], 0);
	gpio_free(cols[i]);
  }
  for(int i = 0; i < 7; i++) {
	gpio_set_value(rows[i], 0);
	gpio_free(rows[i]);
  }
  printk(KERN_INFO "GPIO cleaned up\n");
}

static int __init led_module_init(void) {
  int retval;
  printk(KERN_INFO "LED Matrix Module loading\n");

  /*
   * Create a simple kobject with the name of "led-matrix",
   * located under /sys/kernel/
   *
   * As this is a simple directory, no uevent will be sent to
   * userspace.  That is why this function should not be used for
   * any type of dynamic kobjects, where the name and number are
   * not known ahead of time.
   */
  led_matrix = kobject_create_and_add("led-matrix", kernel_kobj);
  if (!led_matrix) return -ENOMEM;

  /* Create the files associated with this kobject */
  retval = sysfs_create_group(led_matrix, &attr_group);
  if (retval) {
    kobject_put(led_matrix);
    return retval;
  }
  printk(KERN_INFO "Kobject created\n");

  retval = initialize_gpio();
  if (retval) {
	kobject_put(led_matrix);
	return retval;
  }

  gpio_set_value(cols[0], 0);
  gpio_set_value(rows[0], 1);

  repeatingTimer_init();

  printk(KERN_INFO "LED Matrix Module loaded\n");
  return retval;
}

static void __exit led_module_exit(void) {
  kobject_put(led_matrix);
  printk(KERN_INFO "Kobject removed\n");
  cleanup_gpio();
  repeatingTimer_exit();
}

module_init(led_module_init);
module_exit(led_module_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Greg Kroah-Hartman <greg@kroah.com>");
