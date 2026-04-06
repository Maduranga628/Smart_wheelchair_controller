#include "ui_menu.h"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pins_master.h"
#include "motor_stepper.h"
#include "battery.h"
#include "control_cmd.h"

// ---------------- OLED ----------------
static const int OLED_W = 128;
static const int OLED_H = 64;
static const uint8_t OLED_ADDR = 0x3C;

static Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

// ---------------- UI state ----------------
enum ui_state_t {
  UI_STATUS = 0,
  UI_MENU,
  UI_ARM_MENU,
  UI_MODE_MENU
};

static ui_state_t g_ui_state = UI_STATUS;

// Shared state (read by control loop)
static portMUX_TYPE g_ui_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool g_armed = false;
static volatile control_mode_t g_mode = MODE_JOYSTICK;

// Cursors
static int menu_cursor = 0; // 0=ARM/DISARM, 1=SELECT MODE
static int arm_cursor  = 1; // 0=ARM, 1=DISARM (default safest)
static int mode_cursor = 1; // 0=BLE, 1=JOYSTICK, 2=GESTURE, 3=VOICE

// Battery cache
static float g_bat_v = 0.0f;
static uint32_t lastBatMs = 0;

// ---------------- Debounced button ----------------
struct DebouncedButton {
  uint8_t pin = 0;
  bool stable = HIGH;
  bool prevStable = HIGH;
  uint32_t lastEdgeMs = 0;
  bool pressEvent = false;

  void begin(uint8_t p) {
    pin = p;
    pinMode(pin, INPUT_PULLUP);
    stable = digitalRead(pin);
    prevStable = stable;
    lastEdgeMs = millis();
    pressEvent = false;
  }

  void update() {
    bool raw = digitalRead(pin);
    uint32_t now = millis();

    // If raw differs, start debounce timer
    if (raw != stable) {
      if (now - lastEdgeMs >= 35) { // debounce time
        prevStable = stable;
        stable = raw;
        lastEdgeMs = now;

        // Press event: HIGH->LOW (active low)
        if (prevStable == HIGH && stable == LOW) {
          pressEvent = true;
        }
      }
    } else {
      lastEdgeMs = now;
    }
  }

  bool fell() {
    if (pressEvent) {
      pressEvent = false;
      return true;
    }
    return false;
  }
};

static DebouncedButton btnUp, btnDown, btnEnter, btnBack;

// ---------------- Helpers ----------------
static const char* mode_label(control_mode_t m) {
  switch (m) {
    case MODE_BLE:      return "BLE";
    case MODE_JOYSTICK: return "JOYSTICK";
    case MODE_GESTURE:  return "GESTURE";
    case MODE_VOICE:    return "VOICE";
    default:            return "UNKNOWN";
  }
}

static void safe_stop_now() {
  motor_apply_command(CMD_STOP);
}

static void set_armed(bool a) {
  portENTER_CRITICAL(&g_ui_mux);
  g_armed = a;
  portEXIT_CRITICAL(&g_ui_mux);

  if (!a) {
    safe_stop_now();
    motor_enable(false);
  } else {
    motor_enable(true);
    safe_stop_now();
  }
}

static void set_mode(control_mode_t m) {
  portENTER_CRITICAL(&g_ui_mux);
  g_mode = m;
  portEXIT_CRITICAL(&g_ui_mux);

  safe_stop_now();
}

bool ui_is_armed() {
  portENTER_CRITICAL(&g_ui_mux);
  bool a = g_armed;
  portEXIT_CRITICAL(&g_ui_mux);
  return a;
}

control_mode_t ui_get_selected_mode() {
  portENTER_CRITICAL(&g_ui_mux);
  control_mode_t m = (control_mode_t)g_mode;
  portEXIT_CRITICAL(&g_ui_mux);
  return m;
}

// ---------------- Drawing ----------------
static void draw_header(const char* title) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print(title);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

static void draw_status() {
  draw_header("WHEELCHAIR");

  // update battery every 500ms
  uint32_t now = millis();
  if (now - lastBatMs >= 500) {
    lastBatMs = now;
    g_bat_v = battery_read_volts();
  }

  bool armed = ui_is_armed();
  control_mode_t mode = ui_get_selected_mode();

  // Top right battery
  display.setCursor(74, 0);
  display.print(g_bat_v, 1);
  display.print("V");

  display.setCursor(0, 16);
  display.print("ARM: ");
  display.print(armed ? "YES" : "NO");

  display.setCursor(0, 28);
  display.print("MODE: ");
  display.print(mode_label(mode));

  display.setCursor(0, 44);
  display.print("ENTER=MENU");

  display.setCursor(0, 56);
  display.print("BACK=STATUS");

  display.display();
}

static void draw_main_menu() {
  draw_header("MENU");

  display.setCursor(0, 18);
  display.print(menu_cursor == 0 ? "> " : "  ");
  display.print("ARM / DISARM");

  display.setCursor(0, 34);
  display.print(menu_cursor == 1 ? "> " : "  ");
  display.print("SELECT MODE");

  display.setCursor(0, 56);
  display.print("BACK=EXIT");

  display.display();
}

static void draw_arm_menu() {
  draw_header("ARM STATUS");

  display.setCursor(0, 22);
  display.print(arm_cursor == 0 ? "> " : "  ");
  display.print("ARM");

  display.setCursor(0, 36);
  display.print(arm_cursor == 1 ? "> " : "  ");
  display.print("DISARM");

  display.setCursor(0, 56);
  display.print("ENTER=OK");

  display.display();
}

static void draw_mode_menu() {
  draw_header("SELECT MODE");

  const control_mode_t modes[] = { MODE_BLE, MODE_JOYSTICK, MODE_GESTURE, MODE_VOICE };
  const int MODE_COUNT = 4;

  if (mode_cursor < 0) mode_cursor = 0;
  if (mode_cursor >= MODE_COUNT) mode_cursor = MODE_COUNT - 1;

  for (int i = 0; i < MODE_COUNT; i++) {
    display.setCursor(0, 16 + i * 12);
    display.print(i == mode_cursor ? "> " : "  ");
    display.print(mode_label(modes[i]));
  }

  display.setCursor(0, 56);
  display.print("ENTER=OK");

  display.display();
}

// ---------------- UI logic ----------------
static void ui_handle_buttons() {
  btnUp.update();
  btnDown.update();
  btnEnter.update();
  btnBack.update();

  bool up    = btnUp.fell();
  bool down  = btnDown.fell();
  bool enter = btnEnter.fell();
  bool back  = btnBack.fell();

  if (g_ui_state == UI_STATUS) {
    if (enter) {
      g_ui_state = UI_MENU;
      menu_cursor = 0;
    }
    return;
  }

  if (g_ui_state == UI_MENU) {
    if (back) {
      g_ui_state = UI_STATUS;
      return;
    }

    if (up)   menu_cursor = (menu_cursor - 1 + 2) % 2;
    if (down) menu_cursor = (menu_cursor + 1) % 2;

    if (enter) {
      if (menu_cursor == 0) {
        g_ui_state = UI_ARM_MENU;
        arm_cursor = ui_is_armed() ? 0 : 1; // highlight current state
      } else {
        g_ui_state = UI_MODE_MENU;

        // sync cursor to current mode
        control_mode_t cm = ui_get_selected_mode();
        if      (cm == MODE_BLE)      mode_cursor = 0;
        else if (cm == MODE_JOYSTICK) mode_cursor = 1;
        else if (cm == MODE_GESTURE)  mode_cursor = 2;
        else if (cm == MODE_VOICE)    mode_cursor = 3;
        else                          mode_cursor = 1;
      }
    }
    return;
  }

  if (g_ui_state == UI_ARM_MENU) {
    if (back) {
      g_ui_state = UI_MENU;
      return;
    }

    if (up || down) {
      arm_cursor = (arm_cursor == 0) ? 1 : 0;
    }

    if (enter) {
      if (arm_cursor == 0) set_armed(true);
      else set_armed(false);
      g_ui_state = UI_STATUS;
    }
    return;
  }

  if (g_ui_state == UI_MODE_MENU) {
    if (back) {
      g_ui_state = UI_MENU;
      return;
    }

    const int MODE_COUNT = 4;
    if (up)   mode_cursor = (mode_cursor - 1 + MODE_COUNT) % MODE_COUNT;
    if (down) mode_cursor = (mode_cursor + 1) % MODE_COUNT;

    if (enter) {
      if (mode_cursor == 0) set_mode(MODE_BLE);
      if (mode_cursor == 1) set_mode(MODE_JOYSTICK);
      if (mode_cursor == 2) set_mode(MODE_GESTURE);
      if (mode_cursor == 3) set_mode(MODE_VOICE);

      g_ui_state = UI_STATUS;
    }
    return;
  }
}

void ui_init() {
  // Buttons
  btnUp.begin(PIN_BTN_UP);
  btnDown.begin(PIN_BTN_DOWN);
  btnEnter.begin(PIN_BTN_ENTER);
  btnBack.begin(PIN_BTN_BACK);

  // Battery ADC
  battery_init();

  // OLED I2C (IMPORTANT: your pins SDA=7, SCL=6)
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  Wire.setClock(100000); // stable

  // OLED begin
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // OLED failed, keep system safe
    set_armed(false);
    return;
  }

  display.clearDisplay();
  display.display();

  // Defaults: safest
  set_armed(false);
  set_mode(MODE_JOYSTICK);

  // Initial screen
  g_ui_state = UI_STATUS;
  menu_cursor = 0;
  arm_cursor = 1;   // DISARM
  mode_cursor = 1;  // JOYSTICK

  draw_status();
}

void ui_task(void* pv) {
  (void)pv;

  uint32_t lastDraw = 0;

  while (true) {
    ui_handle_buttons();

    uint32_t now = millis();
    if (now - lastDraw >= 80) {
      lastDraw = now;

      switch (g_ui_state) {
        case UI_STATUS:   draw_status();    break;
        case UI_MENU:     draw_main_menu(); break;
        case UI_ARM_MENU: draw_arm_menu();  break;
        case UI_MODE_MENU:draw_mode_menu(); break;
        default:          draw_status();    break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
