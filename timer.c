#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <stddef.h>

#include "matrix.h"

static ktime_t scanlineTimerInterval;
static ktime_t frameTimerInterval;
static struct hrtimer scanlineTimer;
static struct hrtimer frameTimer;
struct task_struct *scanlineThread = NULL;
struct task_struct *frameThread = NULL;

static unsigned long frameNanosec = 1000000000;  // 1 hz frame rate
static unsigned long scanlineNanosec =
    2000000;  // 100 hz total screen refresh rate

static enum hrtimer_restart restartScanlineTimer(struct hrtimer *timer) {
  wake_up_process(scanlineThread);
  hrtimer_forward_now(timer, scanlineTimerInterval);
  return HRTIMER_RESTART;
}

static enum hrtimer_restart restartFrameTimer(struct hrtimer *timer) {
  wake_up_process(frameThread);
  hrtimer_forward_now(timer, frameTimerInterval);
  return HRTIMER_RESTART;
}

static int updateScanLine(void *data) {
  int currentCol = 0;
  while (1) {
    currentCol++;
    if (currentCol <= COLS) {
      matrix_display_col(currentCol - 1);
    } else {
      matrix_display_col(0);
      currentCol = 1;
    }
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
    if (kthread_should_stop()) {
      break;
    }
  }
  printk(KERN_INFO "updateScanLine terminating.\n");
  return 0;
}

static int updateFrame(void *data) {
  //char currentChar = '0';
  while (1) {
    // currentChar++;
    // if (currentChar <= 'z') {
    //   matrix_set_character(currentChar);
    // } else {
    //   currentChar = '0' - 1;
    //   matrix_set_character('0');
    // }
    matrix_display_scroll();
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
    if (kthread_should_stop()) {
      break;
    }
  }
  printk(KERN_INFO "updateFrame terminating.\n");
  return 0;
}

int timer_init(void) {
  printk(KERN_INFO "Repeating Timer module is loaded\n");

  scanlineThread = kthread_run(updateScanLine, NULL, "updateScanLine");
  frameThread = kthread_run(updateFrame, NULL, "updateFrame");
  if ((void*)scanlineThread == ERR_PTR || (void*)frameThread == ERR_PTR) {
    printk(KERN_ALERT "Thread failed to create!\n");
    return EINTR;
  }

  scanlineTimerInterval = ktime_set(0, scanlineNanosec);
  printk(KERN_INFO "Timer initial timer value is %lldms \n",
         ktime_to_ms(scanlineTimerInterval));
  hrtimer_init(&scanlineTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  scanlineTimer.function = restartScanlineTimer;
  hrtimer_start(&scanlineTimer, scanlineTimerInterval, HRTIMER_MODE_REL);

  frameTimerInterval = ktime_set(0, frameNanosec);
  printk(KERN_INFO "Timer initial timer value is %lldms \n",
         ktime_to_ms(frameTimerInterval));
  hrtimer_init(&frameTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  frameTimer.function = restartFrameTimer;
  hrtimer_start(&frameTimer, frameTimerInterval, HRTIMER_MODE_REL);
  return 0;
}

void timer_exit(void) {
  printk(KERN_INFO "Unloading module and attempting to cancel timer\n");
  kthread_stop(scanlineThread);
  kthread_stop(frameThread);
  hrtimer_cancel(&scanlineTimer);
  hrtimer_cancel(&frameTimer);
}

void timer_set_scanline_interval(int sec, unsigned long nsec) {
  scanlineTimerInterval = ktime_set(sec, nsec);
  printk(KERN_INFO "Scanline interval set to %lldms \n",
         ktime_to_ms(scanlineTimerInterval));
  hrtimer_start(&scanlineTimer, scanlineTimerInterval, HRTIMER_MODE_REL);
}

void timer_set_frame_interval(int sec, unsigned long nsec) {
  frameTimerInterval = ktime_set(sec, nsec);
  printk(KERN_INFO "Frame interval set to %lldms \n",
         ktime_to_ms(frameTimerInterval));
  hrtimer_start(&frameTimer, frameTimerInterval, HRTIMER_MODE_REL);
}