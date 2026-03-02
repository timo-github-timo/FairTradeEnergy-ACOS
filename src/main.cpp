#include <M5Unified.h>
#include <lvgl.h>

// --- Display/Touch Parameter ---
static const uint16_t SCREEN_WIDTH  = 320;
static const uint16_t SCREEN_HEIGHT = 240;

// LVGL Draw Buffer (z. B. 40 Zeilen)
static lv_color_t buf1[SCREEN_WIDTH * 40];
static lv_disp_draw_buf_t draw_buf;
static lv_disp_t* disp = nullptr;

// Touch Eingabegerät
static lv_indev_t* indev_touch = nullptr;

// --- UI-Objekte ---
static lv_obj_t* page0 = nullptr;
static lv_obj_t* page1 = nullptr;
static lv_obj_t* label0 = nullptr;
static lv_obj_t* button1 = nullptr;       // auf page1
static lv_obj_t* button2 = nullptr;       // auf page0
static lv_obj_t* button0 = nullptr;       // auf page1
static lv_obj_t* button0_label = nullptr; // Text von button0

static int counter = 0;
static uint32_t last_tick_ms = 0;

// --- LVGL Display Flush (LVGL -> M5 LCD) ---
static void my_disp_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
  const int32_t w = (area->x2 - area->x1 + 1);
  const int32_t h = (area->y2 - area->y1 + 1);

  M5.Display.startWrite();
  M5.Display.setAddrWindow(area->x1, area->y1, w, h);
  M5.Display.pushPixels((uint16_t*)&color_p->full, w * h);
  M5.Display.endWrite();

  lv_disp_flush_ready(drv);
}

// --- LVGL Touch Read (Touch -> LVGL) ---
static void my_touch_read(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  (void)drv;
  auto t = M5.Touch.getDetail();
  if (t.isPressed()) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = t.x;
    data->point.y = t.y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// --- Button Events ---
static void button2_event_handler(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    lv_scr_load(page1); // wie dein button2_released_event: page1 anzeigen
  }
}
static void button1_event_handler(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    lv_scr_load(page0); // wie dein button1_released_event: page0 anzeigen
  }
}

// --- UI aufbauen (entspricht setup() in Python) ---
static void build_ui() {
  page0 = lv_obj_create(nullptr);
  page1 = lv_obj_create(nullptr);

  lv_obj_set_size(page0, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_size(page1, SCREEN_WIDTH, SCREEN_HEIGHT);

  lv_obj_set_style_bg_color(page0, lv_color_hex(0x55a79a), 0);
  lv_obj_set_style_bg_opa(page0, LV_OPA_COVER, 0);

  lv_obj_set_style_bg_color(page1, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(page1, LV_OPA_COVER, 0);

  // label0 auf page0
  label0 = lv_label_create(page0);
  lv_obj_set_pos(label0, 37, 46);
  lv_obj_set_style_text_color(label0, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_color(label0, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(label0, LV_OPA_TRANSP, 0);
  lv_obj_set_style_text_font(label0, &lv_font_montserrat_14, 0);
  lv_label_set_text(label0, "label0");

  // button2 auf page0 (Text "button2") -> page1 laden
  button2 = lv_btn_create(page0);
  lv_obj_set_pos(button2, 228, 201);
  lv_obj_set_size(button2, 80, 32);
  lv_obj_set_style_bg_color(button2, lv_color_hex(0x2196f3), 0);

  lv_obj_t* button2_label = lv_label_create(button2);
  lv_obj_set_style_text_color(button2_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(button2_label, &lv_font_montserrat_14, 0);
  lv_label_set_text(button2_label, "button2");

  lv_obj_add_event_cb(button2, button2_event_handler, LV_EVENT_ALL, nullptr);

  // button1 auf page1 (Text "button0") -> page0 laden
  button1 = lv_btn_create(page1);
  lv_obj_set_pos(button1, 167, 77);
  lv_obj_set_size(button1, 80, 32);
  lv_obj_set_style_bg_color(button1, lv_color_hex(0x2196f3), 0);

  lv_obj_t* button1_label = lv_label_create(button1);
  lv_obj_set_style_text_color(button1_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(button1_label, &lv_font_montserrat_14, 0);
  lv_label_set_text(button1_label, "button0");

  lv_obj_add_event_cb(button1, button1_event_handler, LV_EVENT_ALL, nullptr);

  // button0 auf page1 (Text wird in loop() auf "PRESS ME" gesetzt)
  button0 = lv_btn_create(page1);
  lv_obj_set_pos(button0, 40, 76);
  lv_obj_set_size(button0, 100, 36);
  lv_obj_set_style_bg_color(button0, lv_color_hex(0x2196f3), 0);

  button0_label = lv_label_create(button0);
  lv_obj_set_style_text_color(button0_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(button0_label, &lv_font_montserrat_14, 0);
  lv_label_set_text(button0_label, "button0");

  // Startbildschirm wie in deinem Python
  lv_scr_load(page1);
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // Rotation ~ Widgets.setRotation(1)
  M5.Display.setRotation(1);

  // LVGL init
  lv_init();

  // Draw Buffer
  lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, sizeof(buf1) / sizeof(buf1[0]));

  // Display Treiber registrieren
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.flush_cb = my_disp_flush;
  disp = lv_disp_drv_register(&disp_drv);

  // Touch Treiber registrieren
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  indev_touch = lv_indev_drv_register(&indev_drv);

  build_ui();
  last_tick_ms = millis();
}

void loop() {
  M5.update();

  // LVGL Tick an LVGL melden
  uint32_t now = millis();
  uint32_t elapsed = now - last_tick_ms;
  if (elapsed) {
    lv_tick_inc(elapsed);
    last_tick_ms = now;
  }

  // Logik analog zu deinem Python loop()
  counter += 1;

  // Label-Farbe Schritt 1 (pulsierender Grünanteil)
  uint8_t g = static_cast<uint8_t>(counter % 255);
  lv_obj_set_style_text_color(label0, lv_color_make(0xFF, g, 0x00), 0);

  // Button0 Text
  lv_label_set_text(button0_label, "PRESS ME");

  lv_timer_handler();
  delay(500);

  // Schritt 2 (Basisfarbe 0x3366ff, mit pulsierendem Grün)
  g = static_cast<uint8_t>(counter % 255);
  lv_obj_set_style_text_color(label0, lv_color_make(0x33, g, 0xFF), 0);

  lv_timer_handler();
  delay(500);
}