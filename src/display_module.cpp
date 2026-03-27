// ================================================================
//  display_module.cpp  —  Display & Touch (M5CoreS3SE, 320×240)
// ================================================================

#include "display_module.h"

// ----------------------------------------------------------------
//  Layout  (alle Maße in Pixel, Querformat 320×240)
// ----------------------------------------------------------------
static const int SCR_W    = 320;
static const int SCR_H    = 240;

// Tiles (Kopfzeile)
static const int TILE_Y   = 2;
static const int TILE_H   = 46;
static const int TILE_W   = 77;
static const int TILE_GAP = 3;
static const int TILE_X[4] = { 2, 82, 162, 242 };

// Hauptbereich (Karten)
static const int CARD_Y   = 52;
static const int CARD_H   = 160;
static const int CARD_R   = 8;     // Eckradius

// Statuszeile
static const int STATUS_Y = 215;
static const int STATUS_H = 23;

// AUTO-View: eine große + eine kleine Karte
static const int AUTO_BIG_X  = 2;
static const int AUTO_BIG_W  = 204;
static const int AUTO_SMALL_X = 211;
static const int AUTO_SMALL_W = 107;

// MANUAL-View: drei gleich breite Karten
static const int MAN_X[3] = { 2, 110, 218 };
static const int MAN_W    = 100;

// ----------------------------------------------------------------
//  Farben  (RGB565)
// ----------------------------------------------------------------
static const uint32_t COL_BG         = 0x0820;   // sehr dunkles Navy
static const uint32_t COL_CARD       = 0x1082;   // dunkles Blau-Grau
static const uint32_t COL_CARD_HL    = 0x0A3D;   // hervorgehobene Karte (Navy)
static const uint32_t COL_BORDER     = 0x4208;   // mittleres Grau
static const uint32_t COL_STATUS_BG  = 0x0820;
static const uint32_t COL_TEXT       = TFT_WHITE;
static const uint32_t COL_LABEL      = 0x8C71;   // helles Grau
static const uint32_t COL_ISLAND     = 0x0526;   // dunkles Grün-Blau
static const uint32_t COL_GRID       = 0x0340;   // dunkles Grün
static const uint32_t COL_OFF        = 0x3000;   // dunkles Rot

// ----------------------------------------------------------------
//  Interner Zustand
// ----------------------------------------------------------------
enum DispView { VIEW_AUTO, VIEW_MANUAL };

static DispView    _view         = VIEW_AUTO;
static DisplayData _lastData     = {};
static bool        _needFullDraw = true;

// ----------------------------------------------------------------
//  Hilfsfunktionen — Zeichnen
// ----------------------------------------------------------------

static void drawTile(int idx, const char* label, const char* value) {
    int x = TILE_X[idx];
    int w = (idx == 3) ? 76 : TILE_W;

    M5.Display.fillRoundRect(x, TILE_Y, w, TILE_H, 6, COL_CARD);
    M5.Display.drawRoundRect(x, TILE_Y, w, TILE_H, 6, COL_BORDER);

    // Label (klein, oben)
    M5.Display.setTextColor(COL_LABEL, COL_CARD);
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(TC_DATUM);
    M5.Display.drawString(label, x + w / 2, TILE_Y + 5);

    // Wert (groß, Mitte)
    M5.Display.setTextColor(COL_TEXT, COL_CARD);
    M5.Display.setTextSize(2);
    M5.Display.drawString(value, x + w / 2, TILE_Y + 20);
}

static void drawTiles(const DisplayData& d) {
    char buf[12];

    snprintf(buf, sizeof(buf), "%.1f", d.u_grid);
    drawTile(0, "U_grid [V]", buf);

    snprintf(buf, sizeof(buf), "%.2f", d.f_grid);
    drawTile(1, "F_grid [Hz]", buf);

    snprintf(buf, sizeof(buf), "%d", d.soc);
    drawTile(2, "SoC [%]", buf);

    snprintf(buf, sizeof(buf), "%.1f", d.batt_v);
    drawTile(3, "Battery [V]", buf);
}

static void drawStatusBar(const String& status) {
    M5.Display.fillRect(0, STATUS_Y, SCR_W, STATUS_H, COL_STATUS_BG);
    M5.Display.drawFastHLine(0, STATUS_Y, SCR_W, COL_BORDER);

    M5.Display.setTextColor(COL_LABEL, COL_STATUS_BG);
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(ML_DATUM);
    M5.Display.drawString("Status:  " + status, 6, STATUS_Y + STATUS_H / 2);
}

static void drawCard(int x, int y, int w, int h,
                     uint32_t fill, uint32_t border,
                     const char* topLabel, const char* mainText) {
    M5.Display.fillRoundRect(x, y, w, h, CARD_R, fill);
    M5.Display.drawRoundRect(x, y, w, h, CARD_R, border);

    // "Mode:" Label oben
    M5.Display.setTextColor(COL_LABEL, fill);
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(TC_DATUM);
    M5.Display.drawString(topLabel, x + w / 2, y + 14);

    // Haupttext (groß, vertikal zentriert)
    M5.Display.setTextColor(COL_TEXT, fill);
    M5.Display.setTextSize(2);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString(mainText, x + w / 2, y + h / 2 + 8);
}

// ----------------------------------------------------------------
//  View-Zeichenfunktionen
// ----------------------------------------------------------------

static void drawAutoView() {
    // Großes Karte: AUTO
    drawCard(AUTO_BIG_X, CARD_Y, AUTO_BIG_W, CARD_H,
             COL_CARD, COL_BORDER, "Mode:", "AUTO");

    // Kleine Karte: Manual (Schaltfläche)
    drawCard(AUTO_SMALL_X, CARD_Y, AUTO_SMALL_W, CARD_H,
             COL_CARD, COL_BORDER, "Mode:", "Manual");

    // Pfeil-Hinweis in der kleinen Karte
    M5.Display.setTextColor(COL_LABEL, COL_CARD);
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(BC_DATUM);
    M5.Display.drawString("[ Tap ]",
                          AUTO_SMALL_X + AUTO_SMALL_W / 2,
                          CARD_Y + CARD_H - 10);
}

static void drawManualView() {
    drawCard(MAN_X[0], CARD_Y, MAN_W, CARD_H,
             COL_ISLAND, COL_BORDER, "Mode:", "Island");

    drawCard(MAN_X[1], CARD_Y, MAN_W, CARD_H,
             COL_OFF, COL_BORDER, "Mode:", "OFF");

    drawCard(MAN_X[2], CARD_Y, MAN_W, CARD_H,
             COL_GRID, COL_BORDER, "Mode:", "Grid");

    // Zurück-Hinweis in der Statuszeile
    M5.Display.setTextColor(COL_LABEL, COL_STATUS_BG);
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(MR_DATUM);
    M5.Display.drawString("[ Status antippen → Zurück ]",
                          SCR_W - 4, STATUS_Y + STATUS_H / 2);
}

// ----------------------------------------------------------------
//  Öffentliche Funktionen
// ----------------------------------------------------------------

void display_init() {
    M5.Display.setRotation(1);
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.fillScreen(COL_BG);

    M5.Display.setTextColor(TFT_WHITE, COL_BG);
    M5.Display.setTextSize(2);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.drawString("ACOS", SCR_W / 2, SCR_H / 2 - 16);
    M5.Display.setTextSize(1);
    M5.Display.drawString("Automatic Change-Over Switch",
                           SCR_W / 2, SCR_H / 2 + 8);

    Serial.println("[Display] initialisiert.");
}

void display_update(const DisplayData& data) {
    static uint32_t lastDraw = 0;
    if (!_needFullDraw && millis() - lastDraw < 100) return;
    lastDraw = millis();

    M5.Display.startWrite();

    if (_needFullDraw) {
        M5.Display.fillScreen(COL_BG);
        _needFullDraw = false;
    }

    drawTiles(data);

    if (_view == VIEW_AUTO) {
        drawAutoView();
    } else {
        drawManualView();
    }

    drawStatusBar(data.status);

    M5.Display.endWrite();
    _lastData = data;
}

DispEvent display_handle_touch() {
    auto tp = M5.Touch.getDetail();
    if (!tp.wasPressed()) return DISP_EVT_NONE;

    int tx = tp.x;
    int ty = tp.y;

    if (_view == VIEW_AUTO) {
        // Touch auf die "Manual"-Karte
        if (tx >= AUTO_SMALL_X && tx <= AUTO_SMALL_X + AUTO_SMALL_W &&
            ty >= CARD_Y       && ty <= CARD_Y + CARD_H) {
            _view         = VIEW_MANUAL;
            _needFullDraw = true;
            return DISP_EVT_TO_MANUAL;
        }
    } else {
        // Touch auf Statuszeile → zurück zu AUTO
        if (ty >= STATUS_Y) {
            _view         = VIEW_AUTO;
            _needFullDraw = true;
            return DISP_EVT_TO_AUTO;
        }

        // Touch auf Island
        if (tx >= MAN_X[0] && tx <= MAN_X[0] + MAN_W &&
            ty >= CARD_Y   && ty <= CARD_Y + CARD_H) {
            return DISP_EVT_MODE_ISLAND;
        }

        // Touch auf OFF
        if (tx >= MAN_X[1] && tx <= MAN_X[1] + MAN_W &&
            ty >= CARD_Y   && ty <= CARD_Y + CARD_H) {
            return DISP_EVT_MODE_OFF;
        }

        // Touch auf Grid
        if (tx >= MAN_X[2] && tx <= MAN_X[2] + MAN_W &&
            ty >= CARD_Y   && ty <= CARD_Y + CARD_H) {
            return DISP_EVT_MODE_GRID;
        }
    }

    return DISP_EVT_NONE;
}
