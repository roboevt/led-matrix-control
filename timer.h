
// Initialize two timers, one for the scanlines and one to update the framebuffer.
int timer_init(void);
// Cancel the timers.
void timer_exit(void);
// Set the time to display each scanline.
void timer_set_scanline_interval(int sec, unsigned long nsec);
// Set the delay between frames.
void timer_set_frame_interval(int sec, unsigned long nsec);