#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <stddef.h>

#include "matrix.h"

static ktime_t timer_interval;
static struct hrtimer timer;
struct task_struct *kernelThread = NULL;

static __s64 seconds = 0;
static unsigned long nanoseconds = 1000000;  // 167 hz total screen refresh rate

static enum hrtimer_restart restart_timer(struct hrtimer *timer) {
  wake_up_process(kernelThread);
  hrtimer_forward_now(timer, timer_interval);
  return HRTIMER_RESTART;
}

static int threadFunction(void *data) {
  int currentCol = 0;
  printk(KERN_INFO "threadFunction starting with data %p\n", data);
  while (1) {
    currentCol++;
    if (currentCol <= COLS) {
      matrix_display_col(currentCol - 1);
    } else {
      currentCol = 0;
    }
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
    if (kthread_should_stop()) {
      break;
    }
  }
  printk(KERN_INFO "threadFunction terminating.\n");
  return 0;
}

int timer_init(void) {
  printk(KERN_INFO "Repeating Timer module is loaded\n");

  kernelThread = kthread_run(threadFunction, NULL, "threadFunction");
  if (!kernelThread) {
    printk(KERN_ALERT "Thread failed to create!\n");
    return EINTR;
  }

  timer_interval = ktime_set(seconds, nanoseconds);
  printk(KERN_INFO "Timer initial timer value is %lldms \n",
         ktime_to_ms(timer_interval));
  hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  timer.function = restart_timer;
  hrtimer_start(&timer, timer_interval, HRTIMER_MODE_REL);
  return 0;
}

void timer_exit(void) {
  printk(KERN_INFO "Unloading module and attempting to cancel timer\n");
  kthread_stop(kernelThread);
  hrtimer_cancel(&timer);
}

void timer_set_interval(int sec, unsigned long nsec) {
  timer_interval = ktime_set(sec, nsec);
  printk(KERN_INFO "Timer interval set to %lldms \n",
         ktime_to_ms(timer_interval));
  hrtimer_start(&timer, timer_interval, HRTIMER_MODE_REL);
}