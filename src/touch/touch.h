//touch config header file
#include <M5Unified.h>
#include <lvgl.h>

// --- Display/Touch Parameter ---
const uint16_t SCREEN_WIDTH  = 320;
const uint16_t SCREEN_HEIGHT = 240;

void my_touch_read(lv_indev_drv_t* drv, lv_indev_data_t* data);