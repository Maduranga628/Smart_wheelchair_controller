#include "ble_uart.h"
#include <NimBLEDevice.h>
#include <ctype.h>

// Nordic UART Service UUIDs (common BLE UART profile). :contentReference[oaicite:2]{index=2}
static const char* UUID_NUS_SVC = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char* UUID_NUS_RX  = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // write from phone -> ESP
static const char* UUID_NUS_TX  = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // notify from ESP -> phone

static NimBLEServer*         g_server = nullptr;
static NimBLECharacteristic* g_rxChar = nullptr;
static NimBLECharacteristic* g_txChar = nullptr;

static portMUX_TYPE g_bleMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool         g_connected = false;
static volatile uint32_t     g_lastRxMs  = 0;
static volatile motion_cmd_t g_lastCmd   = CMD_STOP;

static inline uint32_t now_ms() { return (uint32_t)millis(); }

static std::string trim_upper(std::string s) {
  // trim spaces/newlines
  while (!s.empty() && (s.front() == ' ' || s.front() == '\r' || s.front() == '\n' || s.front() == '\t')) s.erase(0,1);
  while (!s.empty() && (s.back()  == ' ' || s.back()  == '\r' || s.back()  == '\n' || s.back()  == '\t')) s.pop_back();

  // uppercase
  for (auto &c : s) c = (char)toupper((unsigned char)c);
  return s;
}

static motion_cmd_t parse_cmd(const std::string& in) {
  std::string s = trim_upper(in);

  // allow things like "F\n" or "F,123" (take first token)
  size_t cut = s.find_first_of(" ,;\t\r\n");
  if (cut != std::string::npos) s = s.substr(0, cut);

  if (s == "F")  return CMD_F;
  if (s == "B")  return CMD_B;
  if (s == "LH") return CMD_LH;
  if (s == "RH") return CMD_RH;
  if (s == "LF") return CMD_LF;
  if (s == "RF") return CMD_RF;
  if (s == "LB") return CMD_LB;
  if (s == "RB") return CMD_RB;
  if (s == "S" || s == "STOP") return CMD_STOP;

  return CMD_STOP;
}

// NimBLE callback signatures (newer NimBLE uses NimBLEConnInfo). :contentReference[oaicite:3]{index=3}
class ServerCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, NimBLEConnInfo& connInfo) override {
    (void)s; (void)connInfo;
    portENTER_CRITICAL(&g_bleMux);
    g_connected = true;
    portEXIT_CRITICAL(&g_bleMux);
  }

  void onDisconnect(NimBLEServer* s, NimBLEConnInfo& connInfo, int reason) override {
    (void)s; (void)connInfo; (void)reason;
    portENTER_CRITICAL(&g_bleMux);
    g_connected = false;
    g_lastCmd  = CMD_STOP;
    g_lastRxMs = 0;
    portEXIT_CRITICAL(&g_bleMux);

    // restart advertising
    NimBLEDevice::getAdvertising()->start();
  }
};

class RxCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* ch, NimBLEConnInfo& connInfo) override {
    (void)connInfo;
    std::string v = ch->getValue();
    motion_cmd_t cmd = parse_cmd(v);

    portENTER_CRITICAL(&g_bleMux);
    g_lastCmd  = cmd;
    g_lastRxMs = now_ms();
    portEXIT_CRITICAL(&g_bleMux);
  }
};

void ble_init(const char* deviceName) {
  NimBLEDevice::init(deviceName);

  // Server
  g_server = NimBLEDevice::createServer();
  g_server->setCallbacks(new ServerCB());

  // Service
  NimBLEService* svc = g_server->createService(UUID_NUS_SVC);

  // RX (Write)
  g_rxChar = svc->createCharacteristic(
    UUID_NUS_RX,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  g_rxChar->setCallbacks(new RxCB());

  // TX (Notify)
  g_txChar = svc->createCharacteristic(
    UUID_NUS_TX,
    NIMBLE_PROPERTY::NOTIFY
  );

  svc->start();

  // Advertising
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(UUID_NUS_SVC);

  // Use scan response and advertise name (API changed vs older examples). :contentReference[oaicite:4]{index=4}
  adv->enableScanResponse(true);
  adv->setName(deviceName);

  // Preferred connection interval: 12..24 => 15ms..30ms (7.5ms min allowed). :contentReference[oaicite:5]{index=5}
  adv->setPreferredParams(12, 24);

  adv->start();

  // Initialize state
  portENTER_CRITICAL(&g_bleMux);
  g_connected = false;
  g_lastCmd   = CMD_STOP;
  g_lastRxMs  = 0;
  portEXIT_CRITICAL(&g_bleMux);
}

bool ble_is_connected() {
  portENTER_CRITICAL(&g_bleMux);
  bool c = g_connected;
  portEXIT_CRITICAL(&g_bleMux);
  return c;
}

bool ble_get_recent_command(motion_cmd_t* outCmd, uint32_t maxAgeMs) {
  uint32_t t;
  motion_cmd_t c;

  portENTER_CRITICAL(&g_bleMux);
  t = g_lastRxMs;
  c = g_lastCmd;
  portEXIT_CRITICAL(&g_bleMux);

  if (t == 0) return false;

  uint32_t age = now_ms() - t;
  if (age > maxAgeMs) return false;

  *outCmd = c;
  return true;
}

void ble_send_text(const char* s) {
  if (!g_txChar) return;
  if (!ble_is_connected()) return;
  g_txChar->setValue((uint8_t*)s, strlen(s));
  g_txChar->notify();
}
