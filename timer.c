#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/kthread.h>

#include "matrix.h"

static ktime_t scanlineTimerInterval;  // How long to hold each scanline
static ktime_t frameTimerInterval;     // How long to hold each frame
static struct hrtimer scanlineTimer;   // The timer for the scanlines
static struct hrtimer frameTimer;      // The timer for the frames
static int currentCol = 0;             // The current column being displayed
struct task_struct *scanlineThread = NULL; // The thread for the scanlines
struct task_struct *frameThread = NULL;    // The thread for the frames

static unsigned long scanlineNanosec =
    2000000;  // will scan the entire screen at 100 hz

// The timer callback functions
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

// Cycle through the scanlines
static int updateScanLine(void *data) {
  while (1) {
    currentCol++;
    if (currentCol <= COLS) {
      matrix_display_col(currentCol - 1);
    } else {
      matrix_display_col(0);
      currentCol = 1;
    }
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();  // Yield to other processes until timer expires again
    if (kthread_should_stop()) {
      break;
    }
  }
  printk(KERN_INFO "updateScanLine terminating.\n");
  return 0;
}

// Cycle through the frames (scrolling)
static int updateFrame(void *data) {
  while (1) {
    matrix_display_scroll();
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();  // Yield to other processes until timer expires again
    if (kthread_should_stop()) {
      break;
    }
  }
  printk(KERN_INFO "updateFrame terminating.\n");
  return 0;
}

int timer_init(void) {
  printk(KERN_INFO "Repeating Timer module is loaded\n");

  // Begin the threads and associate the relavent restart functions
  scanlineThread = kthread_run(updateScanLine, NULL, "updateScanLine");
  frameThread = kthread_run(updateFrame, NULL, "updateFrame");
  if ((void *)scanlineThread == ERR_PTR || (void *)frameThread == ERR_PTR) {
    printk(KERN_ALERT "Thread failed to create!\n");
    return EINTR;
  }

  // Initialize the timers
  scanlineTimerInterval = ktime_set(0, scanlineNanosec);
  printk(KERN_INFO "Timer initial timer value is %lldms \n",
         ktime_to_ms(scanlineTimerInterval));
  hrtimer_init(&scanlineTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  scanlineTimer.function = restartScanlineTimer;
  hrtimer_start(&scanlineTimer, scanlineTimerInterval, HRTIMER_MODE_REL);

  frameTimerInterval = ktime_set(__INT_MAX__, __INT_MAX__);  // start at 0 fps
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