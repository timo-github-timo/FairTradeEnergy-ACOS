// ui framework and logic implementation
#include "ui.h"

// --- LVGL Display Flush (LVGL -> M5 LCD) ---
void my_disp_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
  const int32_t w = (area->x2 - area->x1 + 1);
  const int32_t h = (area->y2 - area->y1 + 1);

  M5.Display.startWrite();
  M5.Display.setAddrWindow(area->x1, area->y1, w, h);
  M5.Display.pushPixels((uint16_t*)&color_p->full, w * h);
  M5.Display.endWrite();

  lv_disp_flush_ready(drv);
}