// touch logic implementation
#include "touch.h"

// --- LVGL Touch Read (Touch -> LVGL) ---
void my_touch_read(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  (void)drv;
  auto t = M5.Touch.getDetail();
  if (t.isPressed()) {
    data->state = LV_INDEV_STATE_PRESSED;
    // Adjust touch coordinates for display rotation (setRotation(1))
    // For rotation=1 (90°), swap and invert coordinates
    data->point.x = t.y;                    // swap x <- y
    data->point.y = SCREEN_WIDTH - t.x;     // invert and adjust
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}