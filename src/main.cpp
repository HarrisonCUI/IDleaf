#ifdef ATTRAX_ROUND_DIAG_ONLY
#include <Arduino.h>
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#include <SPI.h>
#ifndef BOARD_SCREEN_COMBO
#define BOARD_SCREEN_COMBO 501
#endif
#include <TFT_eSPI.h>

namespace {

TFT_eSPI g_tft = TFT_eSPI();

constexpr int kGridX = 240;
constexpr int kGridY = 240;
constexpr int kCellXY = 2;
constexpr int kGenDelayMs = 10;
constexpr uint16_t kMaxGenCount = 500;

uint8_t g_grid[kGridX][kGridY];
uint8_t g_newGrid[kGridX][kGridY];
uint16_t g_genCount = 0;

void drawGrid() {
  uint16_t color = TFT_WHITE;
  for (int16_t x = 1; x < kGridX - 1; x++) {
    for (int16_t y = 1; y < kGridY - 1; y++) {
      if (g_grid[x][y] != g_newGrid[x][y]) {
        color = g_newGrid[x][y] == 1 ? 0xFFFF : 0;
        g_tft.fillRect(kCellXY * x, kCellXY * y, kCellXY, kCellXY, color);
      }
    }
  }
}

void initGrid() {
  for (int16_t x = 0; x < kGridX; x++) {
    for (int16_t y = 0; y < kGridY; y++) {
      g_newGrid[x][y] = 0;

      if (x == 0 || x == kGridX - 1 || y == 0 || y == kGridY - 1) {
        g_grid[x][y] = 0;
      } else {
        g_grid[x][y] = random(3) == 1 ? 1 : 0;
      }
    }
  }
}

int getNumberOfNeighbors(int x, int y) {
  return g_grid[x - 1][y] + g_grid[x - 1][y - 1] + g_grid[x][y - 1] +
         g_grid[x + 1][y - 1] + g_grid[x + 1][y] + g_grid[x + 1][y + 1] +
         g_grid[x][y + 1] + g_grid[x - 1][y + 1];
}

void computeCA() {
  for (int16_t x = 1; x < kGridX; x++) {
    for (int16_t y = 1; y < kGridY; y++) {
      int neighbors = getNumberOfNeighbors(x, y);
      if (g_grid[x][y] == 1 && (neighbors == 2 || neighbors == 3)) {
        g_newGrid[x][y] = 1;
      } else if (g_grid[x][y] == 1) {
        g_newGrid[x][y] = 0;
      }
      if (g_grid[x][y] == 0 && neighbors == 3) {
        g_newGrid[x][y] = 1;
      } else if (g_grid[x][y] == 0) {
        g_newGrid[x][y] = 0;
      }
    }
  }
}

}  // namespace

void setup() {
  g_tft.init();
  g_tft.setRotation(3);
  g_tft.fillScreen(TFT_BLACK);
  g_tft.setTextWrap(false);
  g_tft.setTextSize(1);
  g_tft.setTextColor(TFT_WHITE);
  g_tft.setCursor(0, 0);
}

void loop() {
  g_tft.fillScreen(TFT_BLACK);
  g_tft.setTextSize(2);
  g_tft.setTextColor(TFT_WHITE);
  g_tft.setCursor(40, 5);
  g_tft.println(F("Arduino"));
  g_tft.setCursor(35, 25);
  g_tft.println(F("Cellular"));
  g_tft.setCursor(35, 45);
  g_tft.println(F("Automata"));

  delay(1000);

  g_tft.fillScreen(TFT_BLACK);
  initGrid();
  g_genCount = kMaxGenCount;
  drawGrid();

  for (int gen = 0; gen < g_genCount; gen++) {
    computeCA();
    drawGrid();
    delay(kGenDelayMs);
    for (int16_t x = 1; x < kGridX - 1; x++) {
      for (int16_t y = 1; y < kGridY - 1; y++) {
        g_grid[x][y] = g_newGrid[x][y];
      }
    }
  }
}

#else
#include <Arduino.h>
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#ifdef ATTRAX_ROUND_DISPLAY_PORT
#ifndef BOARD_SCREEN_COMBO
#define BOARD_SCREEN_COMBO 501
#endif
#define USE_TFT_ESPI_LIBRARY
#include <lvgl.h>
#include "roundsidplay/Seeed_Arduino_RoundDisplay-main/src/lv_xiao_round_screen.h"
#include "smart_plant_app.h"
#else
#include <U8g2lib.h>
#endif
#include <esp_camera.h>
#include <mbedtls/base64.h>
#include <driver/i2s.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <NimBLEDevice.h>

namespace {

constexpr char kApName[] = "XIAO-CAM-TEST-3";
constexpr uint32_t kLedBlinkMs = 300;
constexpr uint32_t kOledRefreshMs = 500;
constexpr uint32_t kCameraXclkHz = 20000000;
constexpr uint8_t kCameraWarmupFrames = 2;

constexpr int kCameraPinPwdn = -1;
constexpr int kCameraPinReset = -1;
constexpr int kCameraPinXclk = 10;
constexpr int kCameraPinSiod = 40;
constexpr int kCameraPinSioc = 39;
constexpr int kCameraPinY9 = 48;
constexpr int kCameraPinY8 = 11;
constexpr int kCameraPinY7 = 12;
constexpr int kCameraPinY6 = 14;
constexpr int kCameraPinY5 = 16;
constexpr int kCameraPinY4 = 18;
constexpr int kCameraPinY3 = 17;
constexpr int kCameraPinY2 = 15;
constexpr int kCameraPinVsync = 38;
constexpr int kCameraPinHref = 47;
constexpr int kCameraPinPclk = 13;

constexpr int kMicPinWs = 42;
constexpr int kMicPinSd = 41;
constexpr uint32_t kMicSampleRate = 16000;
constexpr int kSoilMoisturePin = A0;
constexpr int kUltrasonicSigPinTx = 43;  // Right-bottom Grove UART port TX6 / D6
constexpr int kUltrasonicSigPinRx = 44;  // Right-bottom Grove UART port RX7 / D7
constexpr int kBuzzerPin = A3;
constexpr int kBuzzerChannel = 1;
constexpr int kBuzzerVolumeDuty = 5; // 0-255, smaller value means lower volume (5 is very quiet)
constexpr uint8_t kDht20Address = 0x38;
constexpr uint32_t kSensorPollMs = 2000;
#ifdef ATTRAX_ROUND_DISPLAY_PORT
constexpr int kDisplayBacklightPin = D6;
constexpr int kDisplayBacklightChannel = 2;
constexpr int kDisplayWidth = 240;
constexpr int kDisplayHeight = 240;
#endif
constexpr uint16_t kSoilDryRaw = 3200;
constexpr uint16_t kSoilWetRaw = 1200;
constexpr uint32_t kUltrasonicTimeoutUs = 30000;

// --- BLE Constants ---
constexpr char kBleDeviceName[] = "XIAO-LED-BLE-3";
constexpr char kBleServiceUuid[] = "19B10000-E8F2-537E-4F6C-D104768A1214";
constexpr char kBleLedCharUuid[] = "19B10001-E8F2-537E-4F6C-D104768A1214";
constexpr char kBleSensorCharUuid[] = "19B10002-E8F2-537E-4F6C-D104768A1214";
constexpr char kBleFileCmdCharUuid[] = "19B10003-E8F2-537E-4F6C-D104768A1214";
constexpr char kBleFileDataCharUuid[] = "19B10004-E8F2-537E-4F6C-D104768A1214";

NimBLEServer* g_bleServer = nullptr;
NimBLECharacteristic* g_bleLedChar = nullptr;
NimBLECharacteristic* g_bleSensorChar = nullptr;
NimBLECharacteristic* g_bleFileCmdChar = nullptr;
NimBLECharacteristic* g_bleFileDataChar = nullptr;
bool g_bleConnected = false;
bool g_startBleSync = false;
bool g_startBleCapture = false;
bool g_startBleRecord = false;
bool g_bleShouldAdvertise = false;
unsigned long g_bleDisconnectTime = 0;
unsigned long g_lastBleNotifyMs = 0;
constexpr uint32_t kBleNotifyIntervalMs = 2000;

unsigned long g_lastProximityCaptureMs = 0;
constexpr uint32_t kProximityCaptureCooldownMs = 20000; // 20 seconds
constexpr float kProximityThresholdCm = 50.0;

WebServer g_server(80);
#ifdef ATTRAX_ROUND_DISPLAY_PORT
lv_obj_t* g_uiRoot = nullptr;
lv_obj_t* g_uiModeOverlay = nullptr;
#else
// CRITICAL: Must use Software I2C. Hardware I2C (even Wire1) will cause a fatal panic/deadlock with Camera/I2S.
// Reverted back to SSD1306 as SSD1315 caused a blank screen on this specific board revision.
U8G2_SSD1306_128X64_NONAME_F_SW_I2C g_u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
#endif

bool g_oledReady = false;
bool g_cameraReady = false;
bool g_cameraInitAttempted = false;
bool g_micReady = false;
uint32_t g_micRms = 0;
unsigned long g_lastLoudSoundTime = 0;
const uint32_t kLoudSoundThreshold = 3000; // RMS阈值
const unsigned long kLoudSoundCooldown = 60000; // 冷却时间1分钟
int16_t g_micSamples[96] = {0};
bool g_ledOn = false;
unsigned long g_lastLedToggleMs = 0;
unsigned long g_lastOledRefreshMs = 0;
unsigned long g_lastOledBreathingMs = 0;
unsigned long g_oledBreathingCycleMs = 4000;
uint8_t g_oledMaxBrightness = 255;
uint8_t g_oledDisplayMode = 0; // 0: 数据文字, 1: 全黑息屏, 2: 半屏纯白, 3: 全屏纯白
bool g_autoProximityOled = true; // 默认开启雷达互动模式，可由Web/BLE/HTTP控制
#ifdef ATTRAX_ROUND_DISPLAY_PORT
bool g_loveBreathingActive = false;
unsigned long g_loveBreathingUntilMs = 0;
unsigned long g_savedBreathingCycleMs = 0;
uint8_t g_savedDisplayMode = 0;
uint8_t g_savedMaxBrightness = 255;
uint32_t g_careGiftSeq = 0;
String g_careGiftFrom = "Partner";
String g_careGiftKind = "heart";
String g_careGiftBits = "";
#endif
unsigned long g_lastProximityUpdateMs = 0;
unsigned long g_lastPersonDetectedMs = 0;
unsigned long g_lastSensorPollMs = 0;
String g_cameraError = "pending";
String g_cameraSensor = "unknown";
String g_apStatus = "pending";
bool g_dht20Ready = false;
uint16_t g_soilRaw = 0;
int g_soilPercent = -1;
float g_airTemperatureC = NAN;
float g_airHumidityPct = NAN;
float g_distanceCm = NAN;
String g_dht20Status = "pending";
String g_ultrasonicStatus = "pending";
int g_ultrasonicActivePin = -1;

// --- Plant Audio Engine Data Model ---
struct RhythmStep {
  uint16_t freq;
  uint16_t onMs;
  uint16_t offMs;
};

struct RhythmPattern {
  const char* id;
  const RhythmStep* steps;
  size_t stepCount;
  uint16_t cyclePauseMs;
};

bool g_sdCardMounted = false;
#define MAX_SD_STEPS 200
RhythmStep g_sdPatternSteps[MAX_SD_STEPS];
RhythmPattern g_sdPattern; // To hold the currently loaded SD pattern
String g_currentSdId = "";

constexpr RhythmStep kCalmHeartbeatSteps[] = {{800, 70, 90}, {800, 90, 700}};
constexpr RhythmStep kSlowBreathSteps[] = {{600, 120, 300}, {600, 120, 1200}};
constexpr RhythmStep kGentleRhythmSteps[] = {{1000, 80, 120}, {1000, 80, 200}, {1000, 160, 900}};

#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_D6 1175
#define NOTE_E6 1319

constexpr RhythmStep kTwinkleStarSteps[] = {
  {NOTE_C4, 200, 50}, {NOTE_C4, 200, 50},
  {NOTE_G4, 200, 50}, {NOTE_G4, 200, 50},
  {NOTE_A4, 200, 50}, {NOTE_A4, 200, 50},
  {NOTE_G4, 400, 100},
  {NOTE_F4, 200, 50}, {NOTE_F4, 200, 50},
  {NOTE_E4, 200, 50}, {NOTE_E4, 200, 50},
  {NOTE_D4, 200, 50}, {NOTE_D4, 200, 50},
  {NOTE_C4, 400, 500}
};

constexpr RhythmStep kXiaoMeiManSteps[] = {
  // "没有什么大愿望" (加入了琶音伪和弦)
  {NOTE_E5, 150, 50}, {NOTE_G5, 150, 50}, {NOTE_A5, 150, 50}, 
  {NOTE_E5, 30, 0}, {NOTE_G5, 30, 0}, {NOTE_C6, 240, 50}, // 琶音 C大调
  {NOTE_C6, 150, 50}, {NOTE_A5, 150, 50}, 
  {NOTE_C5, 30, 0}, {NOTE_E5, 30, 0}, {NOTE_G5, 340, 200}, // 琶音 C大调
  
  // "没有什么事要赶" 
  {NOTE_E5, 150, 50}, {NOTE_G5, 150, 50}, {NOTE_A5, 150, 50},
  {NOTE_E5, 30, 0}, {NOTE_G5, 30, 0}, {NOTE_C6, 240, 50}, // 琶音 C大调
  {NOTE_D6, 150, 50}, {NOTE_C6, 150, 50}, 
  {NOTE_C5, 30, 0}, {NOTE_F5, 30, 0}, {NOTE_A5, 340, 200}, // 琶音 F大调
  
  // "看见路口红灯一直闪"
  {NOTE_E5, 150, 50}, {NOTE_G5, 150, 50}, {NOTE_A5, 150, 50},
  {NOTE_C6, 200, 50}, {NOTE_C6, 200, 50}, {NOTE_A5, 150, 50}, {NOTE_G5, 150, 50}, {NOTE_E5, 150, 50}, 
  {NOTE_C5, 30, 0}, {NOTE_E5, 30, 0}, {NOTE_G5, 340, 600}  // 琶音 C大调
};

constexpr RhythmPattern kPatterns[] = {
  {"calm_heartbeat", kCalmHeartbeatSteps, 2, 1500},
  {"slow_breath", kSlowBreathSteps, 2, 2000},
  {"gentle_rhythm", kGentleRhythmSteps, 3, 1800},
  {"twinkle_star", kTwinkleStarSteps, 14, 2000},
  {"xiao_mei_man", kXiaoMeiManSteps, 33, 2000}
};
constexpr size_t kNumPatterns = sizeof(kPatterns) / sizeof(kPatterns[0]);

bool g_audioEnabled = false;
bool g_audioPlaying = false;
String g_audioMode = "manual"; 
String g_audioPatternId = "calm_heartbeat";
unsigned long g_audioLastTransitionMs = 0;
size_t g_audioStepIndex = 0;
bool g_audioOutputHigh = false;
bool g_audioInCyclePause = false;

unsigned long g_audioLastPlayMs = 0;
uint32_t g_audioScheduleIntervalMs = 30 * 60 * 1000; // 30 min

uint32_t g_audioSensorCooldownMs = 2 * 60 * 60 * 1000; // 2 hours
unsigned long g_audioLastSensorTriggerMs = 0;
int g_audioSoilThreshold = 30; // 30%

void initSD() {
  if (!SD.begin(21)) {
    Serial.println("SD Card Mount Failed");
    g_sdCardMounted = false;
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    g_sdCardMounted = false;
    return;
  }
  g_sdCardMounted = true;
  Serial.println("SD Card Initialized.");
  
  if (!SD.exists("/rhythms")) {
    SD.mkdir("/rhythms");
  }
}

bool loadPatternFromSD(String id) {
  if (!g_sdCardMounted) return false;
  String path = "/rhythms/" + id + ".txt";
  File file = SD.open(path);
  if (!file) return false;
  
  size_t stepCount = 0;
  while (file.available() && stepCount < MAX_SD_STEPS) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("//")) continue;
    
    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma + 1);
    if (firstComma > 0 && secondComma > firstComma) {
      g_sdPatternSteps[stepCount].freq = line.substring(0, firstComma).toInt();
      g_sdPatternSteps[stepCount].onMs = line.substring(firstComma + 1, secondComma).toInt();
      g_sdPatternSteps[stepCount].offMs = line.substring(secondComma + 1).toInt();
      stepCount++;
    }
  }
  file.close();
  
  if (stepCount > 0) {
    g_currentSdId = "sd:" + id;
    g_sdPattern.id = g_currentSdId.c_str();
    g_sdPattern.steps = g_sdPatternSteps;
    g_sdPattern.stepCount = stepCount;
    g_sdPattern.cyclePauseMs = 2000;
    return true;
  }
  return false;
}

const RhythmPattern* getPattern(const String& id) {
  if (id.startsWith("sd:")) {
    String sdId = id.substring(3);
    if (loadPatternFromSD(sdId)) {
      return &g_sdPattern;
    }
  }
  for (size_t i = 0; i < kNumPatterns; ++i) {
    if (String(kPatterns[i].id) == id) return &kPatterns[i];
  }
  return &kPatterns[0];
}

void startAudioPlayback() {
  if (g_audioPlaying) return;
  g_audioPlaying = true;
  g_audioStepIndex = 0;
  g_audioOutputHigh = true;
  g_audioInCyclePause = false;
  g_audioLastTransitionMs = millis();
  g_audioLastPlayMs = g_audioLastTransitionMs;
  const RhythmPattern* pat = getPattern(g_audioPatternId);
  ledcWriteTone(kBuzzerPin, pat->steps[0].freq);
  ledcWrite(kBuzzerPin, kBuzzerVolumeDuty);
}

void stopAudioPlayback() {
  g_audioPlaying = false;
  g_audioOutputHigh = false;
  g_audioInCyclePause = false;
  ledcWriteTone(kBuzzerPin, 0);
  ledcWrite(kBuzzerPin, 0);
}

void updateAudioEngine() {
  unsigned long now = millis();

  if (!g_audioPlaying && g_audioEnabled) {
    if (g_audioMode == "scheduled") {
      if (now - g_audioLastPlayMs >= g_audioScheduleIntervalMs) {
        startAudioPlayback();
      }
    } else if (g_audioMode == "sensor_linked") {
      if (g_soilPercent >= 0 && g_soilPercent < g_audioSoilThreshold) {
        if (now - g_audioLastSensorTriggerMs >= g_audioSensorCooldownMs) {
          g_audioLastSensorTriggerMs = now;
          startAudioPlayback();
        }
      }
    }
  }

  if (g_audioPlaying) {
    const RhythmPattern* pat = getPattern(g_audioPatternId);
    
    if (g_audioInCyclePause) {
      if (now - g_audioLastTransitionMs >= pat->cyclePauseMs) {
        stopAudioPlayback();
      }
    } else {
      const RhythmStep& step = pat->steps[g_audioStepIndex];
      uint16_t currentDuration = g_audioOutputHigh ? step.onMs : step.offMs;
      
      if (now - g_audioLastTransitionMs >= currentDuration) {
        g_audioLastTransitionMs = now;
        
        if (g_audioOutputHigh) {
          g_audioOutputHigh = false;
          ledcWriteTone(kBuzzerPin, 0);
          ledcWrite(kBuzzerPin, 0);
        } else {
          g_audioStepIndex++;
          if (g_audioStepIndex >= pat->stepCount) {
            g_audioInCyclePause = true;
          } else {
            g_audioOutputHigh = true;
            ledcWriteTone(kBuzzerPin, pat->steps[g_audioStepIndex].freq);
            ledcWrite(kBuzzerPin, kBuzzerVolumeDuty);
          }
        }
      }
    }
  }
}
// ------------------------------------------

class BleServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    g_bleConnected = true;
    pServer->updateConnParams(connInfo.getConnHandle(), 12, 24, 0, 100); // connection interval min 15ms, max 30ms
    Serial.println("BLE Client Connected");
    // 可选：为了支持多设备同时连接，可以在连接后继续广播
    // NimBLEDevice::startAdvertising(); 
  }
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    g_bleConnected = false;
    Serial.println("BLE Client Disconnected");
    // 不要在回调中直接调用 startAdvertising，因为协议栈可能还在清理资源
    // 让主循环稍微延迟后再重启广播
    g_bleShouldAdvertise = true;
    g_bleDisconnectTime = millis();
  }
};

void setBuiltinLed(bool on);

class BleLedCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      if (value[0] == 0x05 && value.length() >= 2) {
        g_autoProximityOled = (value[1] != 0);
        Serial.printf("BLE Auto Proximity: %d\n", g_autoProximityOled);
      } else if (value[0] == 0x04 && value.length() >= 2) {
        g_autoProximityOled = false; // Disable auto mode when manual command received
        g_oledMaxBrightness = (uint8_t)value[1];
        Serial.printf("BLE OLED Max Brightness: %d\n", g_oledMaxBrightness);
      } else if (value[0] == 0x03 && value.length() >= 2) {
        g_autoProximityOled = false;
        g_oledDisplayMode = (uint8_t)value[1];
        if (g_oledDisplayMode > 3) g_oledDisplayMode = 0;
        g_lastOledRefreshMs = 0; // force redraw
        Serial.printf("BLE OLED Display Mode: %d\n", g_oledDisplayMode);
      } else if (value[0] == 0x02 && value.length() >= 5) {
        g_autoProximityOled = false;
        // 0x02 + 4 bytes uint32_t (little endian)
        uint32_t speed = (uint8_t)value[1] | 
                         ((uint8_t)value[2] << 8) | 
                         ((uint8_t)value[3] << 16) | 
                         ((uint8_t)value[4] << 24);
        g_oledBreathingCycleMs = speed;
        Serial.printf("BLE OLED Breathing Speed: %lu ms\n", g_oledBreathingCycleMs);
      } else if (value[0] == 0x01 || value == "1" || value == "on") {
        setBuiltinLed(true);
        Serial.println("BLE LED: ON");
      } else if (value[0] == 0x00 || value == "0" || value == "off") {
        setBuiltinLed(false);
        Serial.println("BLE LED: OFF");
      }
    }
  }
};

class BleFileCmdCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string value = pCharacteristic->getValue();
    String strVal = String(value.c_str());
    strVal.trim();
    if (strVal == "SYNC") {
      Serial.println("BLE Sync Command Received.");
      g_startBleSync = true;
    } else if (strVal == "CAPTURE") {
      Serial.println("BLE Capture Command Received.");
      g_startBleCapture = true;
    } else if (strVal == "RECORD") {
      Serial.println("BLE Record Command Received.");
      g_startBleRecord = true;
    } else {
      Serial.print("Unknown BLE Command: ");
      Serial.println(strVal);
    }
  }
};

void setupBLE() {
  NimBLEDevice::init(kBleDeviceName);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(false, false, false);
  NimBLEDevice::setMTU(512);

  g_bleServer = NimBLEDevice::createServer();
  g_bleServer->setCallbacks(new BleServerCallbacks());

  NimBLEService* pService = g_bleServer->createService(kBleServiceUuid);

  g_bleLedChar = pService->createCharacteristic(
      kBleLedCharUuid,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
  );
  g_bleLedChar->setCallbacks(new BleLedCallbacks());
  g_bleLedChar->setValue(g_ledOn ? "1" : "0");

  g_bleSensorChar = pService->createCharacteristic(
      kBleSensorCharUuid,
      NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
  );

  g_bleFileCmdChar = pService->createCharacteristic(
      kBleFileCmdCharUuid,
      NIMBLE_PROPERTY::WRITE
  );
  g_bleFileCmdChar->setCallbacks(new BleFileCmdCallbacks());

  g_bleFileDataChar = pService->createCharacteristic(
      kBleFileDataCharUuid,
      NIMBLE_PROPERTY::NOTIFY
  );

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(kBleServiceUuid);
  
  NimBLEDevice::startAdvertising();
  Serial.println("BLE Advertising started.");
}

void notifyBleSensors() {
  if (!g_bleConnected) return;

  unsigned long now = millis();
  if (now - g_lastBleNotifyMs >= kBleNotifyIntervalMs) {
    g_lastBleNotifyMs = now;
    String json = "{";
    json += "\"soil\":" + String(g_soilPercent) + ",";
    json += "\"dist\":" + (isnan(g_distanceCm) ? "null" : String(g_distanceCm, 1)) + ",";
    json += "\"temp\":" + (isnan(g_airTemperatureC) ? "null" : String(g_airTemperatureC, 1)) + ",";
    json += "\"hum\":" + (isnan(g_airHumidityPct) ? "null" : String(g_airHumidityPct, 1)) + ",";
    json += "\"mic\":" + String(g_micRms);
    json += "}";
    g_bleSensorChar->setValue((uint8_t*)json.c_str(), json.length());
    g_bleSensorChar->notify();
  }
}

void setBuiltinLed(bool on) {
  g_ledOn = on;
  digitalWrite(LED_BUILTIN, on ? LOW : HIGH);
}

String ipToString(const IPAddress& ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

int computeSoilPercent(uint16_t raw) {
  const long percent = map(raw, kSoilDryRaw, kSoilWetRaw, 0, 100);
  return constrain(percent, 0, 100);
}

bool writeI2cCommand(uint8_t address, const uint8_t* data, size_t length) {
  Wire.beginTransmission(address);
  for (size_t i = 0; i < length; ++i) {
    Wire.write(data[i]);
  }
  return Wire.endTransmission() == 0;
}

bool readDht20(float& humidityPct, float& temperatureC) {
  const uint8_t trigger[] = {0xAC, 0x33, 0x00};
  if (!writeI2cCommand(kDht20Address, trigger, sizeof(trigger))) {
    g_dht20Status = "trigger fail";
    return false;
  }

  delay(90);

  constexpr uint8_t kResponseSize = 7;
  uint8_t data[kResponseSize] = {0};
  const uint8_t bytesRead = Wire.requestFrom((int)kDht20Address, (int)kResponseSize);
  if (bytesRead != kResponseSize) {
    g_dht20Status = "read fail";
    return false;
  }

  for (uint8_t i = 0; i < kResponseSize; ++i) {
    data[i] = Wire.read();
  }

  if ((data[0] & 0x80) != 0) {
    g_dht20Status = "busy";
    return false;
  }

  const uint32_t humidityRaw =
      (static_cast<uint32_t>(data[1]) << 12) |
      (static_cast<uint32_t>(data[2]) << 4) |
      (static_cast<uint32_t>(data[3]) >> 4);
  const uint32_t temperatureRaw =
      ((static_cast<uint32_t>(data[3]) & 0x0F) << 16) |
      (static_cast<uint32_t>(data[4]) << 8) |
      static_cast<uint32_t>(data[5]);

  humidityPct = (static_cast<float>(humidityRaw) * 100.0f) / 1048576.0f;
  temperatureC = (static_cast<float>(temperatureRaw) * 200.0f) / 1048576.0f - 50.0f;
  g_dht20Status = "ok";
  return true;
}

bool setupDht20() {
  Wire.begin(SDA, SCL, 100000);
  Wire.setTimeOut(50);
  delay(40);

  Wire.beginTransmission(kDht20Address);
  const bool found = (Wire.endTransmission() == 0);
  if (!found) {
    g_dht20Status = "not found";
    Serial.println("DHT20 not found on I2C");
    return false;
  }

  const uint8_t initCmd[] = {0xBE, 0x08, 0x00};
  if (!writeI2cCommand(kDht20Address, initCmd, sizeof(initCmd))) {
    g_dht20Status = "init fail";
    Serial.println("DHT20 init command failed");
    return false;
  }

  g_dht20Ready = true;
  g_dht20Status = "ready";
  Serial.println("DHT20 initialized");
  return true;
}

float readUltrasonicDistanceCmOnPin(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(2);
  digitalWrite(pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pin, LOW);

  pinMode(pin, INPUT);
  
  // Custom pulse reading to handle "too close" edge case where 
  // the echo pulse goes HIGH before standard pulseIn() can start.
  uint32_t startWait = micros();
  while (digitalRead(pin) == LOW) {
    if (micros() - startWait > 5000) {
      return NAN; // Timeout waiting for echo to start
    }
  }
  
  uint32_t startPulse = micros();
  while (digitalRead(pin) == HIGH) {
    if (micros() - startPulse > kUltrasonicTimeoutUs) {
      return NAN; // Timeout waiting for echo to end (out of range)
    }
  }
  
  uint32_t durationUs = micros() - startPulse;
  if (durationUs == 0) return NAN;

  return static_cast<float>(durationUs) / 29.0f / 2.0f;
}

float readUltrasonicDistanceCm() {
  if (g_ultrasonicActivePin >= 0) {
    const float cachedPinDistance = readUltrasonicDistanceCmOnPin(g_ultrasonicActivePin);
    if (!isnan(cachedPinDistance)) {
      g_ultrasonicStatus = g_ultrasonicActivePin == kUltrasonicSigPinTx ? "ok tx6" : "ok rx7";
      return cachedPinDistance;
    }
  }

  const float txDistance = readUltrasonicDistanceCmOnPin(kUltrasonicSigPinTx);
  if (!isnan(txDistance)) {
    g_ultrasonicActivePin = kUltrasonicSigPinTx;
    g_ultrasonicStatus = "ok tx6";
    return txDistance;
  }

  const float rxDistance = readUltrasonicDistanceCmOnPin(kUltrasonicSigPinRx);
  if (!isnan(rxDistance)) {
    g_ultrasonicActivePin = kUltrasonicSigPinRx;
    g_ultrasonicStatus = "ok rx7";
    return rxDistance;
  }

  g_ultrasonicActivePin = -1;
  g_ultrasonicStatus = "timeout";
  return NAN;
}

void pollSensors() {
  const unsigned long now = millis();
  if (now - g_lastSensorPollMs < kSensorPollMs) {
    return;
  }
  g_lastSensorPollMs = now;

#ifdef ATTRAX_ROUND_DISPLAY_PORT
  // Round Display occupies A0/D0 for battery detect and D6/D7 for display backlight/touch IRQ.
  // First migration keeps camera, mic, DHT20, BLE, AP, SD and screen, so these conflicting
  // external sensors are temporarily disabled until rewired.
  g_soilRaw = 0;
  g_soilPercent = -1;
  g_distanceCm = NAN;
  g_ultrasonicActivePin = -1;
  g_ultrasonicStatus = "disabled";
#else
  g_soilRaw = analogRead(kSoilMoisturePin);
  g_soilPercent = computeSoilPercent(g_soilRaw);
  g_distanceCm = readUltrasonicDistanceCm();
#endif

  if (!g_dht20Ready) {
    g_dht20Ready = setupDht20();
  }
  if (!g_dht20Ready) {
    return;
  }

  float humidityPct = NAN;
  float temperatureC = NAN;
  if (readDht20(humidityPct, temperatureC)) {
    g_airHumidityPct = humidityPct;
    g_airTemperatureC = temperatureC;
  }
}

#ifdef ATTRAX_ROUND_DISPLAY_PORT
void setDisplayBacklight(uint8_t value) {
  ledcWrite(kDisplayBacklightPin, value);
}

void startHeartBacklight() {
  if (!g_loveBreathingActive) {
    g_savedBreathingCycleMs = g_oledBreathingCycleMs;
    g_savedDisplayMode = g_oledDisplayMode;
    g_savedMaxBrightness = g_oledMaxBrightness;
  }
  g_loveBreathingActive = true;
  g_loveBreathingUntilMs = millis() + 8000;
  g_oledDisplayMode = 0;
  g_oledMaxBrightness = 255;
  g_oledBreathingCycleMs = 900;
}

void rememberCareGift(const String& sender, const String& kind, const String& bits) {
  g_careGiftSeq++;
  g_careGiftFrom = sender;
  g_careGiftKind = kind;
  g_careGiftBits = bits;
}

void receivePartnerHeart(const String& sender) {
  rememberCareGift(sender, "heart", "");
  startHeartBacklight();
  smartPlantReceiveHeart(sender.c_str());
  g_lastOledRefreshMs = 0;
}

void receivePartnerSketch(const String& sender, const String& bits) {
  rememberCareGift(sender, "sketch", bits);
  startHeartBacklight();
  smartPlantReceiveSketch(sender.c_str(), bits.c_str());
  g_lastOledRefreshMs = 0;
}

void updatePartnerHeartEffect() {
  if (!g_loveBreathingActive) return;
  if (static_cast<long>(millis() - g_loveBreathingUntilMs) < 0) return;
  g_loveBreathingActive = false;
  g_oledBreathingCycleMs = g_savedBreathingCycleMs;
  g_oledDisplayMode = g_savedDisplayMode;
  g_oledMaxBrightness = g_savedMaxBrightness;
  g_lastOledRefreshMs = 0;
}

void createRoundUi() {
  lv_obj_t* screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x07130f), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  g_uiRoot = smartPlantCreate(screen);

  g_uiModeOverlay = lv_obj_create(screen);
  lv_obj_remove_style_all(g_uiModeOverlay);
  lv_obj_set_size(g_uiModeOverlay, kDisplayWidth, kDisplayHeight);
  lv_obj_set_pos(g_uiModeOverlay, 0, 0);
  lv_obj_add_flag(g_uiModeOverlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(g_uiModeOverlay, LV_OBJ_FLAG_SCROLLABLE);
}

void updateRoundLvglUi() {
  if (!g_uiRoot) return;

  if (g_oledDisplayMode == 1) {
    lv_obj_add_flag(g_uiRoot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_uiModeOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(g_uiModeOverlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_uiModeOverlay, LV_OPA_COVER, 0);
    return;
  }

  if (g_oledDisplayMode == 2 || g_oledDisplayMode == 3) {
    lv_obj_add_flag(g_uiRoot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_uiModeOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(g_uiModeOverlay, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(g_uiModeOverlay, g_oledDisplayMode == 2 ? LV_OPA_60 : LV_OPA_COVER, 0);
    return;
  }

  lv_obj_clear_flag(g_uiRoot, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(g_uiModeOverlay, LV_OBJ_FLAG_HIDDEN);
  smartPlantUpdate(g_soilPercent, g_airTemperatureC, g_airHumidityPct,
                   static_cast<uint8_t>(WiFi.softAPgetStationNum()));
}
#endif

void updateOled() {
  if (!g_oledReady) return;

  const unsigned long now = millis();
  if (now - g_lastOledRefreshMs < kOledRefreshMs) {
    return;
  }
  g_lastOledRefreshMs = now;

#ifdef ATTRAX_ROUND_DISPLAY_PORT
  updateRoundLvglUi();
#else
  g_u8g2.clearBuffer();

  if (g_oledDisplayMode == 1) {
    // 1: 全黑息屏 (直接 skip 绘制内容)
  } else if (g_oledDisplayMode == 2) {
    // 2: 半屏纯白 (下方一半)
    g_u8g2.drawBox(0, 32, 128, 32);
  } else if (g_oledDisplayMode == 3) {
    // 3: 全屏纯白
    g_u8g2.drawBox(0, 0, 128, 64);
  } else {
    // 0: 数据文字
    g_u8g2.setFont(u8g2_font_5x8_tr);
    g_u8g2.setCursor(0, 8);
    g_u8g2.setCursor(0, 20);
    g_u8g2.print("IP: ");
    g_u8g2.print(ipToString(WiFi.softAPIP()));
    g_u8g2.setCursor(0, 18);
    g_u8g2.setCursor(0, 30);
    g_u8g2.print("CAM: ");
    if (g_cameraReady) {
      g_u8g2.print(g_cameraSensor);
    } else {
      g_u8g2.print("FAIL");
    }
    g_u8g2.setCursor(0, 28);
    g_u8g2.setCursor(0, 40);
    g_u8g2.print("MIC: ");
    if (g_micReady) {
      g_u8g2.print("RMS ");
      g_u8g2.print(g_micRms);
    } else {
      g_u8g2.print("FAIL");
    }
    g_u8g2.setCursor(0, 38);
    g_u8g2.setCursor(0, 50);
    g_u8g2.print("SOIL: ");
    if (g_soilPercent >= 0) {
      g_u8g2.print(g_soilPercent);
      g_u8g2.print("% ");
    } else {
      g_u8g2.print("N/A");
    }

    g_u8g2.setCursor(0, 48);
    g_u8g2.print("TH: ");
    if (g_dht20Ready && !isnan(g_airTemperatureC) && !isnan(g_airHumidityPct)) {
      g_u8g2.print(g_airTemperatureC, 1);
      g_u8g2.print("C ");
      g_u8g2.print(g_airHumidityPct, 0);
      g_u8g2.print("%");
    } else {
      g_u8g2.print(g_dht20Status);
    }

    g_u8g2.setCursor(0, 58);
    g_u8g2.print("DIST: ");
    if (!isnan(g_distanceCm)) {
      g_u8g2.print(g_distanceCm, 0);
      g_u8g2.print("cm");
    } else {
      g_u8g2.print(g_ultrasonicStatus);
    }
    
    g_u8g2.setCursor(64, 58);
    g_u8g2.print("S:");
    if (g_audioPlaying) {
      g_u8g2.print("PLAY");
    } else if (!g_audioEnabled) {
      g_u8g2.print("OFF");
    } else {
      if (g_audioMode == "manual") g_u8g2.print("MAN");
      else if (g_audioMode == "scheduled") g_u8g2.print("SCH");
      else g_u8g2.print("LNK");
    }
  }

  g_u8g2.sendBuffer();
#endif
}

void updateOledBreathing() {
  if (!g_oledReady) return;
  unsigned long now = millis();
  if (now - g_lastOledBreathingMs >= 50) { // 50ms update rate (20Hz)
    g_lastOledBreathingMs = now;
#ifdef ATTRAX_ROUND_DISPLAY_PORT
    if (g_oledDisplayMode == 1 && g_oledBreathingCycleMs == 0) {
      setDisplayBacklight(0);
      return;
    }
    if (g_oledBreathingCycleMs == 0) {
      setDisplayBacklight(g_oledMaxBrightness);
      return;
    }
#else
    if (g_oledBreathingCycleMs == 0) {
      g_u8g2.setPowerSave(0); // Ensure display is on
      g_u8g2.setContrast(g_oledMaxBrightness);
      return;
    }
#endif

    // 核心心跳算法 (Heartbeat waveform)
    // 根据周期判断，如果是急促周期 (<1500ms)，模拟心跳双击 (噗通...噗通...)
    float adjusted_val = 0.0;
    float phase = (now % g_oledBreathingCycleMs) / (float)g_oledBreathingCycleMs; // 0.0 to 1.0

    if (g_oledBreathingCycleMs <= 1500) {
      // 心跳模式: 包含两个紧凑的波峰 (Systole 和 Diastole)，中间夹杂全黑
      // 第一跳：0.0 - 0.2
      // 第二跳：0.3 - 0.5
      // 休息：0.5 - 1.0
      if (phase < 0.2) {
        adjusted_val = sin(phase / 0.2 * PI); // 半个正弦波 0 -> 1 -> 0
      } else if (phase >= 0.3 && phase < 0.5) {
        adjusted_val = sin((phase - 0.3) / 0.2 * PI) * 0.8; // 第二跳稍微弱一点
      } else {
        adjusted_val = 0.0; // 休息期全黑
      }
      
      // 锐化明暗对比
      adjusted_val = (adjusted_val - 0.1) * 1.2; 
    } else {
      // 平缓呼吸模式 (Slow Breathing)
      float val = (sin(phase * 2.0 * PI - PI/2.0) + 1.0) / 2.0;
      // 截断放大：波谷(<0.2)全黑，波峰(>0.8)满亮度，中间明暗渐变
      adjusted_val = (val - 0.2) * (1.0 / 0.6); 
    }

    if (adjusted_val < 0.0) adjusted_val = 0.0;
    if (adjusted_val > 1.0) adjusted_val = 1.0;

#ifdef ATTRAX_ROUND_DISPLAY_PORT
    const uint8_t brightness =
        adjusted_val <= 0.01 ? 0 : (uint8_t)(adjusted_val * (g_oledMaxBrightness - 1.0) + 1.0);
    setDisplayBacklight(brightness);
#else
    if (adjusted_val <= 0.01) {
      // 真正的全黑（关闭显示面板输出）
      g_u8g2.setPowerSave(1);
    } else {
      // 开启显示面板输出，并设置对比度
      g_u8g2.setPowerSave(0);
      uint8_t contrast = (uint8_t)(adjusted_val * (g_oledMaxBrightness - 1.0) + 1.0);
      g_u8g2.setContrast(contrast);
    }
#endif
  }
}

bool setupMic() {
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
      .sample_rate = kMicSampleRate,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 1024,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_PIN_NO_CHANGE,
      .ws_io_num = kMicPinWs,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = kMicPinSd
  };

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, nullptr);
  if (err != ESP_OK) {
    Serial.printf("I2S install failed: %d\n", err);
    return false;
  }

  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("I2S set pin failed: %d\n", err);
    return false;
  }

  g_micReady = true;
  Serial.println("Microphone (I2S) initialized");
  return true;
}

void pollMic() {
  if (!g_micReady) return;

  size_t bytesRead = 0;
  int16_t rawBuf[1024];
  // 0 timeout so we don't block the loop
  esp_err_t err = i2s_read(I2S_NUM_0, rawBuf, sizeof(rawBuf), &bytesRead, 0);
  
  if (err == ESP_OK && bytesRead > 0) {
    int samplesRead = bytesRead / 2;
    
    // 1. Calculate DC offset
    int64_t sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += rawBuf[i];
    }
    int16_t dcOffset = sum / samplesRead;

    // 2. Remove DC offset, apply software gain, and calculate RMS
    uint64_t sumSq = 0;
    int step = samplesRead / 96;
    if (step < 1) step = 1;
    
    const int gain = 15; // Software gain multiplier

    for (int i = 0; i < samplesRead; i++) {
      int32_t adjusted = (rawBuf[i] - dcOffset) * gain;
      // Clamp to 16-bit range
      if (adjusted > 32767) adjusted = 32767;
      if (adjusted < -32768) adjusted = -32768;
      
      sumSq += (uint64_t)adjusted * adjusted;
      
      // Update samples for Web UI visualization
      if (i % step == 0 && (i / step) < 96) {
        g_micSamples[i / step] = adjusted;
      }
    }
    g_micRms = sqrt(sumSq / samplesRead);
  }
}

bool setupCamera() {
  g_cameraInitAttempted = true;
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = kCameraPinY2;
  config.pin_d1 = kCameraPinY3;
  config.pin_d2 = kCameraPinY4;
  config.pin_d3 = kCameraPinY5;
  config.pin_d4 = kCameraPinY6;
  config.pin_d5 = kCameraPinY7;
  config.pin_d6 = kCameraPinY8;
  config.pin_d7 = kCameraPinY9;
  config.pin_xclk = kCameraPinXclk;
  config.pin_pclk = kCameraPinPclk;
  config.pin_vsync = kCameraPinVsync;
  config.pin_href = kCameraPinHref;
  config.pin_sccb_sda = kCameraPinSiod;
  config.pin_sccb_scl = kCameraPinSioc;
  config.pin_pwdn = kCameraPinPwdn;
  config.pin_reset = kCameraPinReset;
  config.xclk_freq_hz = kCameraXclkHz;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  const esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "init 0x%04x", static_cast<unsigned int>(err));
    g_cameraError = buffer;
    Serial.printf("Camera init failed: %s\n", g_cameraError.c_str());
    return false;
  }

  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    if (sensor->id.PID == OV3660_PID) {
      g_cameraSensor = "OV3660";
      sensor->set_vflip(sensor, 1);
      sensor->set_brightness(sensor, 1);
      sensor->set_saturation(sensor, -2);
    } else if (sensor->id.PID == OV2640_PID) {
      g_cameraSensor = "OV2640";
    } else {
      g_cameraSensor = "PID 0x" + String(sensor->id.PID, HEX);
    }
    sensor->set_framesize(sensor, FRAMESIZE_QVGA);
  }

  Serial.printf("Camera ready, sensor=%s, psram=%s\n", g_cameraSensor.c_str(), psramFound() ? "yes" : "no");
  g_cameraError = "";
  return true;
}

bool ensureCameraReady() {
  if (g_cameraReady) {
    return true;
  }
  if (g_cameraInitAttempted) {
    return false;
  }
  g_cameraReady = setupCamera();
  return g_cameraReady;
}

camera_fb_t* captureFrame(bool colorbar) {
  if (!ensureCameraReady()) {
    return nullptr;
  }

  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor == nullptr) {
    return nullptr;
  }

  if (colorbar) {
    sensor->set_colorbar(sensor, 1);
    delay(120);
  }

  for (uint8_t i = 0; i < kCameraWarmupFrames; ++i) {
    camera_fb_t* warmup = esp_camera_fb_get();
    if (warmup != nullptr) {
      esp_camera_fb_return(warmup);
    }
  }

  camera_fb_t* fb = esp_camera_fb_get();

  if (colorbar) {
    sensor->set_colorbar(sensor, 0);
  }

  if (fb != nullptr) {
    Serial.printf(
        "Capture OK: %u bytes, %ux%u\n",
        static_cast<unsigned int>(fb->len),
        static_cast<unsigned int>(fb->width),
        static_cast<unsigned int>(fb->height));
  }
  return fb;
}

void sendFrameJson(bool colorbar) {
  camera_fb_t* fb = captureFrame(colorbar);
  if (fb == nullptr) {
    g_server.send(500, "application/json", "{\"error\":\"capture failed\"}");
    return;
  }

  size_t output_len = 0;
  mbedtls_base64_encode(nullptr, 0, &output_len, fb->buf, fb->len);

  uint8_t* base64_buf = (uint8_t*)malloc(output_len + 1);
  if (base64_buf == nullptr) {
    esp_camera_fb_return(fb);
    g_server.send(500, "application/json", "{\"error\":\"out of memory\"}");
    return;
  }

  size_t olen = 0;
  mbedtls_base64_encode(base64_buf, output_len, &olen, fb->buf, fb->len);
  base64_buf[olen] = '\0'; // ensure null-terminated

  String json;
  json.reserve(olen + 100);
  json += "{\"image\":\"data:image/jpeg;base64,";
  json += (char*)base64_buf;
  json += "\",\"width\":";
  json += fb->width;
  json += ",\"height\":";
  json += fb->height;
  json += "}";

  free(base64_buf);
  esp_camera_fb_return(fb);

  g_server.send(200, "application/json", json);
}

void sendFrame(bool colorbar) {
  camera_fb_t* fb = captureFrame(colorbar);
  if (fb == nullptr) {
    g_server.send(500, "text/plain", colorbar ? "colorbar failed" : "capture failed");
    return;
  }

  g_server.setContentLength(fb->len);
  g_server.send(200, "image/jpeg", "");
  WiFiClient client = g_server.client();
  client.write(fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

String getSDFilesOptions() {
  if (!g_sdCardMounted) return "";
  String options = "";
  File root = SD.open("/rhythms");
  if (!root || !root.isDirectory()) return options;
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String fileName = file.name();
      // On ESP32, file.name() sometimes includes the full path (e.g. /rhythms/song.txt)
      int slash = fileName.lastIndexOf('/');
      if (slash >= 0) fileName = fileName.substring(slash + 1);
      
      if (fileName.endsWith(".txt")) {
        String id = fileName.substring(0, fileName.length() - 4);
        options += "<option value='sd:" + id + "'>[SD] " + id + "</option>";
      }
    }
    file = root.openNextFile();
  }
  return options;
}

String buildIndexHtml() {
  String html;
  html.reserve(10500);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>IDleaf Care Console</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5;color:#111;}";
  html += ".card{background:#fff;border-radius:12px;padding:16px;max-width:520px;margin:auto;box-shadow:0 2px 10px rgba(0,0,0,.08);}";
  html += "button{padding:10px 14px;margin-right:8px;margin-bottom:8px;}";
  html += "img{width:100%;min-height:180px;background:#111;border-radius:8px;object-fit:contain;}";
  html += "canvas{width:100%;height:100px;background:#222;border-radius:8px;margin-top:10px;touch-action:none;}";
  html += "#drawPad{height:190px;background:#07130f;border:3px solid #4de395;box-sizing:border-box;}";
  html += ".sensor{margin:8px 0;padding:10px;background:#f0f3f7;border-radius:8px;}";
  html += ".care{background:#07130f;color:#e5faee;border:2px solid #4de395;}";
  html += ".care input{padding:10px;border-radius:8px;border:1px solid #4de395;margin:6px 0;max-width:180px;}";
  html += ".care button{border:0;border-radius:999px;font-weight:bold;}";
  html += ".pill{display:inline-block;padding:4px 8px;border-radius:999px;background:#e8fff2;color:#166534;margin:2px 0;}";
  html += "code{font-size:13px;}p{line-height:1.4;}</style></head><body><div class='card'>";
  html += "<h2>IDleaf Care Console</h2>";
  html += "<p>AP: <code>" + String(kApName) + "</code> | IP: <code>" + ipToString(WiFi.softAPIP()) + "</code></p>";
#ifdef ATTRAX_ROUND_DISPLAY_PORT
  html += "<div class='sensor care'><strong>Care Together - Draw & Send</strong><p>在下面画爱心或小图，然后发送到圆屏。</p>";
  html += "<input id='heartFrom' value='Luna' maxlength='16' placeholder='Your name'> ";
  html += "<button onclick='sendHeart()' style='background:#ff6685;color:white'>Send heart &lt;3</button>";
  html += "<canvas id='drawPad' width='160' height='120'></canvas>";
  html += "<button onclick='drawHeartPad()' style='background:#ff6685;color:white'>Draw heart</button>";
  html += "<button onclick='clearPad()' style='background:#e5faee;color:#07130f'>Clear</button>";
  html += "<button onclick='sendSketch()' style='background:#4de395;color:#07130f'>Send drawing</button>";
  html += "<div id='heartState'></div><div id='inboxState' class='pill'>Waiting for care gifts...</div>";
  html += "<hr><strong>Interactive Judge Demo</strong><p>Use fake data and click interactions during the presentation.</p>";
  html += "<button onclick='startPlantDemo()' style='background:#a7f6c8;color:#07130f'>Auto demo</button>";
  html += "<button onclick='demoAction(\"good\")' style='background:#4de395;color:#07130f'>Fake good</button>";
  html += "<button onclick='demoAction(\"dry\")' style='background:#f3c45b;color:#07130f'>Fake dry</button>";
  html += "<button onclick='demoAction(\"heart\")' style='background:#ff6685;color:white'>Fake heart</button>";
  html += "<button onclick='demoAction(\"sketch\")' style='background:#e5faee;color:#07130f'>Fake sketch</button>";
  html += "</div>";
#endif
  html += "<h3>Hardware Status</h3>";
  html += "<p>Camera: <code>" + (g_cameraReady ? g_cameraSensor : g_cameraError) + "</code></p>";
  html += "<p>Mic Ready: <code>" + String(g_micReady ? "true" : "false") + "</code></p>";
  html += "<div class='sensor'><strong>Soil Moisture</strong><div id='soilValue'>Loading...</div></div>";
  html += "<div class='sensor'><strong>Temperature / Humidity</strong><div id='thValue'>Loading...</div></div>";
  html += "<div class='sensor'><strong>Ultrasonic Distance</strong><div id='distValue'>Loading...</div></div>";
  
  html += "<div class='sensor'><strong>OLED Breathing Speed</strong><br>";
  html += "<input type='range' id='oledSpeed' min='0' max='10000' step='500' value='4000' oninput='document.getElementById(\"oledSpeedVal\").textContent=this.value+\" ms\";' onchange='setOledBreathing()'>";
  html += " <span id='oledSpeedVal'>4000 ms</span>";
  html += "</div>";
  html += "<div class='sensor' style='margin-top:0;'>";
  html += "<label><input type='checkbox' id='oledWhiteCb' onchange='setOledWhiteMode()'> 全屏纯白呼吸模式 (无文字)</label>";
  html += "</div>";
  
  html += "<div><button onclick='loadImg(\"/capture\")'>Capture</button>";
  html += "<button onclick='loadImg(\"/capture-colorbar\")'>Colorbar</button>";
  html += "<button onclick='fetch(\"/led/on\",{method:\"POST\"})'>LED ON</button>";
  html += "<button onclick='fetch(\"/led/off\",{method:\"POST\"})'>LED OFF</button></div>";
  html += "<div class='sensor'><strong>Plant Audio (Buzzer)</strong>";
  html += "<div id='audioState' style='margin-bottom:8px'>Loading...</div>";
  html += "<select id='audioMode' onchange='setAudioConfig()'><option value='manual'>Manual</option><option value='scheduled'>Scheduled</option><option value='sensor_linked'>Sensor Linked</option></select> ";
  html += "<select id='audioPattern' onchange='setAudioConfig()'><option value='calm_heartbeat'>Calm Heartbeat</option><option value='slow_breath'>Slow Breath</option><option value='gentle_rhythm'>Gentle Rhythm</option><option value='twinkle_star'>Twinkle Star</option><option value='xiao_mei_man'>Xiao Mei Man</option>";
  html += getSDFilesOptions();
  html += "</select><br><br>";
  html += "<button onclick='fetch(\"/audio/enable\",{method:\"POST\"}).then(()=>pollAudio())'>Enable</button>";
  html += "<button onclick='fetch(\"/audio/disable\",{method:\"POST\"}).then(()=>pollAudio())'>Disable</button>";
  html += "<button onclick='fetch(\"/audio/test\",{method:\"POST\"}).then(()=>pollAudio())'>Test Play</button>";
  html += "</div>";

  html += "<img id='cam' alt='capture'>";
  html += "<canvas id='micWave'></canvas>";
  html += "<script>async function loadImg(path){const img=document.getElementById('cam');try{const r=await fetch(path+'?t='+Date.now(),{cache:'no-store'});if(!r.ok)throw new Error('capture');const data=await r.json();img.src=data.image;}catch(e){alert('capture failed');}}";
  html += "const ctx=document.getElementById('micWave').getContext('2d');";
  html += "async function pollSensors(){try{const r=await fetch('/sensors');const d=await r.json();";
  html += "document.getElementById('soilValue').textContent='Raw: '+d.soil_raw+' | Approx: '+d.soil_percent+'%';";
  html += "document.getElementById('thValue').textContent=d.dht20_has_reading?('Temp: '+d.temperature_c.toFixed(1)+' C | Humidity: '+d.humidity_pct.toFixed(1)+'%'):('Status: '+d.dht20_status);";
  html += "document.getElementById('distValue').textContent=d.ultrasonic_has_reading?('Distance: '+d.distance_cm.toFixed(1)+' cm'):('Status: '+d.ultrasonic_status);";
  html += "}catch(e){}setTimeout(pollSensors,2000);}";
  html += "function setOledBreathing(){const v=document.getElementById('oledSpeed').value; document.getElementById('oledSpeedVal').textContent=v+' ms'; fetch('/oled/breathing?cycle_ms='+v,{method:'POST'});}";
  html += "function setOledWhiteMode(){const w=document.getElementById('oledWhiteCb').checked; fetch('/oled/mode?white='+(w?'1':'0'),{method:'POST'});}";
  html += "async function pollAudio(){try{const r=await fetch('/audio-status');const d=await r.json();";
  html += "document.getElementById('audioState').textContent=(d.enabled?'Enabled':'Disabled')+' | Mode: '+d.mode+' | Playing: '+d.playing;";
  html += "document.getElementById('audioMode').value=d.mode;";
  html += "document.getElementById('audioPattern').value=d.pattern;";
  html += "}catch(e){}}";
  html += "async function setAudioConfig(){const m=document.getElementById('audioMode').value;const p=document.getElementById('audioPattern').value;await fetch('/audio/config?mode='+m+'&pattern='+p,{method:'POST'});pollAudio();}";
#ifdef ATTRAX_ROUND_DISPLAY_PORT
  html += "let giftSeq=0;const draw=document.getElementById('drawPad');const dctx=draw.getContext('2d');let drawing=false;";
  html += "function clearPad(){dctx.fillStyle='#07130f';dctx.fillRect(0,0,draw.width,draw.height);}";
  html += "function pos(e){const r=draw.getBoundingClientRect();const p=e.touches?e.touches[0]:e;return{x:(p.clientX-r.left)*draw.width/r.width,y:(p.clientY-r.top)*draw.height/r.height};}";
  html += "function beginDraw(e){drawing=true;const p=pos(e);dctx.beginPath();dctx.moveTo(p.x,p.y);e.preventDefault();}";
  html += "function moveDraw(e){if(!drawing)return;const p=pos(e);dctx.strokeStyle='#ff6685';dctx.lineWidth=10;dctx.lineCap='round';dctx.lineTo(p.x,p.y);dctx.stroke();e.preventDefault();}";
  html += "function endDraw(){drawing=false;}";
  html += "draw.addEventListener('mousedown',beginDraw);draw.addEventListener('mousemove',moveDraw);window.addEventListener('mouseup',endDraw);";
  html += "draw.addEventListener('touchstart',beginDraw,{passive:false});draw.addEventListener('touchmove',moveDraw,{passive:false});draw.addEventListener('touchend',endDraw);clearPad();";
  html += "function drawHeartPad(){clearPad();dctx.strokeStyle='#ff6685';dctx.lineWidth=9;dctx.lineCap='round';dctx.beginPath();dctx.moveTo(80,92);dctx.bezierCurveTo(18,54,42,18,80,42);dctx.bezierCurveTo(118,18,142,54,80,92);dctx.stroke();}";
  html += "function sketchBits(){const img=dctx.getImageData(0,0,draw.width,draw.height).data;let bits='';for(let y=0;y<8;y++){for(let x=0;x<8;x++){let ink=0;for(let yy=0;yy<12;yy+=3){for(let xx=0;xx<20;xx+=4){const px=Math.floor((x+xx/20)*draw.width/8);const py=Math.floor((y+yy/12)*draw.height/8);const i=(py*draw.width+px)*4;if(img[i]>80||img[i+1]>80||img[i+2]>80)ink++;}}bits+=ink>2?'1':'0';}}return bits;}";
  html += "async function sendHeart(){const n=encodeURIComponent(document.getElementById('heartFrom').value||'Partner');";
  html += "const r=await fetch('/plant/heart?from='+n,{method:'POST'});";
  html += "document.getElementById('heartState').textContent=r.ok?'Heart sent to display!':'Send failed';}";
  html += "async function sendSketch(){const n=encodeURIComponent(document.getElementById('heartFrom').value||'Partner');const bits=sketchBits();";
  html += "const r=await fetch('/plant/sketch?from='+n+'&bits='+bits,{method:'POST'});";
  html += "document.getElementById('heartState').textContent=r.ok?'Drawing sent to display!':'Send failed';}";
  html += "async function startPlantDemo(){const r=await fetch('/plant/demo',{method:'POST'});";
  html += "document.getElementById('heartState').textContent=r.ok?'Demo started on display':'Demo failed';}";
  html += "async function demoAction(a){const r=await fetch('/plant/demo-event?action='+a,{method:'POST'});document.getElementById('heartState').textContent=r.ok?'Demo action: '+a:'Demo action failed';}";
  html += "async function pollInbox(){try{const r=await fetch('/plant/inbox');const d=await r.json();if(d.seq&&d.seq!==giftSeq){giftSeq=d.seq;document.getElementById('inboxState').textContent='Latest '+d.kind+' from '+d.from;}}catch(e){}setTimeout(pollInbox,1500);}pollInbox();";
#endif
  html += "setInterval(pollAudio, 2000); pollAudio();";
  html += "async function pollMic(){try{const r=await fetch('/audio-frame');const d=await r.json();if(d.ok&&d.samples){";
  html += "const w=ctx.canvas.width;const h=ctx.canvas.height;ctx.clearRect(0,0,w,h);ctx.beginPath();ctx.strokeStyle='#0f0';ctx.lineWidth=2;";
  html += "for(let i=0;i<d.samples.length;i++){const x=(i/(d.samples.length-1))*w;";
  html += "const y=h/2-(d.samples[i]/32768)*(h/2);if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}ctx.stroke();";
  html += "}}catch(e){}setTimeout(pollMic,100);}pollMic();pollSensors();</script>";
  html += "</div></body></html>";
  return html;
}

void setupWebServer() {
  g_server.on("/", HTTP_GET, []() {
    Serial.println("GET /");
    g_server.send(200, "text/html", buildIndexHtml());
  });
  g_server.on("/capture", HTTP_GET, []() {
    Serial.println("GET /capture");
    sendFrameJson(false);
  });
  g_server.on("/capture-colorbar", HTTP_GET, []() {
    Serial.println("GET /capture-colorbar");
    sendFrameJson(true);
  });
  g_server.on("/audio-frame", HTTP_GET, []() {
    String json;
    json.reserve(1024);
    json += "{\"ok\":";
    json += g_micReady ? "true" : "false";
    json += ",\"rms\":";
    json += g_micRms;
    json += ",\"samples\":[";
    for (int i = 0; i < 96; i++) {
      json += g_micSamples[i];
      if (i < 95) json += ",";
    }
    json += "]}";
    g_server.send(200, "application/json", json);
  });
  g_server.on("/sensors", HTTP_GET, []() {
    String json;
    json.reserve(256);
    json += "{\"soil_raw\":";
    json += g_soilRaw;
    json += ",\"soil_percent\":";
    json += g_soilPercent;
    json += ",\"dht20_ready\":";
    json += g_dht20Ready ? "true" : "false";
    json += ",\"dht20_has_reading\":";
    json += (!isnan(g_airTemperatureC) && !isnan(g_airHumidityPct)) ? "true" : "false";
    json += ",\"temperature_c\":";
    if (isnan(g_airTemperatureC)) {
      json += "null";
    } else {
      json += String(g_airTemperatureC, 1);
    }
    json += ",\"humidity_pct\":";
    if (isnan(g_airHumidityPct)) {
      json += "null";
    } else {
      json += String(g_airHumidityPct, 1);
    }
    json += ",\"dht20_status\":\"";
    json += g_dht20Status;
    json += "\",\"ultrasonic_has_reading\":";
    json += !isnan(g_distanceCm) ? "true" : "false";
    json += ",\"distance_cm\":";
    if (isnan(g_distanceCm)) {
      json += "null";
    } else {
      json += String(g_distanceCm, 1);
    }
    json += ",\"ultrasonic_status\":\"";
    json += g_ultrasonicStatus;
    json += "\"}";
    g_server.send(200, "application/json", json);
  });
  g_server.on("/led/on", HTTP_POST, []() {
    Serial.println("POST /led/on");
    setBuiltinLed(true);
    g_server.send(200, "application/json", "{\"ok\":true}");
  });
  g_server.on("/led/off", HTTP_POST, []() {
    Serial.println("POST /led/off");
    setBuiltinLed(false);
    g_server.send(200, "application/json", "{\"ok\":true}");
  });
  g_server.on("/oled/mode", HTTP_POST, []() {
    g_autoProximityOled = false;
    if (g_server.hasArg("mode")) {
      g_oledDisplayMode = (uint8_t)g_server.arg("mode").toInt();
      if (g_oledDisplayMode > 3) g_oledDisplayMode = 0;
      g_lastOledRefreshMs = 0; // force redraw
      Serial.printf("OLED Display Mode set to %d\n", g_oledDisplayMode);
    } else if (g_server.hasArg("white")) { // 向后兼容
      g_oledDisplayMode = (g_server.arg("white") == "1" || g_server.arg("white") == "true") ? 3 : 0;
      g_lastOledRefreshMs = 0;
      Serial.printf("OLED Display Mode (legacy white) set to %d\n", g_oledDisplayMode);
    }
    g_server.send(200, "application/json", "{\"ok\":true}");
  });
  g_server.on("/oled/brightness", HTTP_POST, []() {
    g_autoProximityOled = false;
    if (g_server.hasArg("max_brightness")) {
      g_oledMaxBrightness = (uint8_t)g_server.arg("max_brightness").toInt();
      Serial.printf("OLED Max Brightness set to %d\n", g_oledMaxBrightness);
    }
    g_server.send(200, "application/json", "{\"ok\":true}");
  });
  g_server.on("/oled/breathing", HTTP_POST, []() {
    g_autoProximityOled = false;
    if (g_server.hasArg("cycle_ms")) {
      g_oledBreathingCycleMs = g_server.arg("cycle_ms").toInt();
      Serial.printf("OLED breathing cycle set to %lu ms\n", g_oledBreathingCycleMs);
    }
    g_server.send(200, "application/json", "{\"ok\":true}");
  });
  g_server.on("/oled/proximity", HTTP_POST, []() {
    if (g_server.hasArg("enable")) {
      g_autoProximityOled = (g_server.arg("enable") == "1" || g_server.arg("enable") == "true");
      Serial.printf("OLED Auto Proximity set to %d\n", g_autoProximityOled);
    }
    g_server.send(200, "application/json", "{\"ok\":true}");
  });
#ifdef ATTRAX_ROUND_DISPLAY_PORT
  g_server.on("/plant/heart", HTTP_POST, []() {
    String sender = g_server.hasArg("from") ? g_server.arg("from") : "Partner";
    sender.trim();
    if (sender.length() == 0) sender = "Partner";
    if (sender.length() > 16) sender = sender.substring(0, 16);
    receivePartnerHeart(sender);
    Serial.printf("Plant heart received from %s\n", sender.c_str());
    g_server.send(200, "application/json", "{\"ok\":true,\"effect\":\"heartbeat\"}");
  });
  g_server.on("/plant/sketch", HTTP_POST, []() {
    String sender = g_server.hasArg("from") ? g_server.arg("from") : "Partner";
    String bits = g_server.hasArg("bits") ? g_server.arg("bits") : "";
    sender.trim();
    bits.trim();
    if (sender.length() == 0) sender = "Partner";
    if (sender.length() > 16) sender = sender.substring(0, 16);
    String clean;
    clean.reserve(64);
    for (uint16_t i = 0; i < bits.length() && clean.length() < 64; i++) {
      clean += bits[i] == '1' ? '1' : '0';
    }
    while (clean.length() < 64) clean += '0';
    receivePartnerSketch(sender, clean);
    Serial.printf("Plant sketch received from %s\n", sender.c_str());
    g_server.send(200, "application/json", "{\"ok\":true,\"effect\":\"sketch\"}");
  });
  g_server.on("/plant/demo", HTTP_POST, []() {
    smartPlantStartDemo();
    Serial.println("Plant judge demo started");
    g_server.send(200, "application/json", "{\"ok\":true,\"demo\":\"started\"}");
  });
  g_server.on("/plant/demo-event", HTTP_POST, []() {
    String action = g_server.hasArg("action") ? g_server.arg("action") : "next";
    action.trim();
    if (action.length() == 0) action = "next";
    smartPlantDemoAction(action.c_str());
    Serial.printf("Plant demo action: %s\n", action.c_str());
    g_server.send(200, "application/json", "{\"ok\":true}");
  });
  g_server.on("/plant/inbox", HTTP_GET, []() {
    String json;
    json.reserve(160);
    json += "{\"seq\":";
    json += g_careGiftSeq;
    json += ",\"from\":\"";
    json += g_careGiftFrom;
    json += "\",\"kind\":\"";
    json += g_careGiftKind;
    json += "\",\"bits\":\"";
    json += g_careGiftBits;
    json += "\"}";
    g_server.send(200, "application/json", json);
  });
#endif
  g_server.on("/status", HTTP_GET, []() {
    Serial.println("GET /status");
    String json = "{\"camera_ready\":";
    json += g_cameraReady ? "true" : "false";
    json += ",\"sensor\":\"" + g_cameraSensor + "\",\"ap_status\":\"" + g_apStatus + "\",\"led_pin\":" + String(LED_BUILTIN) + "}";
    g_server.send(200, "application/json", json);
  });

  g_server.on("/audio-status", HTTP_GET, []() {
    String json = "{\"enabled\":";
    json += g_audioEnabled ? "true" : "false";
    json += ",\"playing\":";
    json += g_audioPlaying ? "true" : "false";
    json += ",\"mode\":\"" + g_audioMode + "\"";
    json += ",\"pattern\":\"" + g_audioPatternId + "\"";
    json += "}";
    g_server.send(200, "application/json", json);
  });

  g_server.on("/audio/test", HTTP_POST, []() {
    Serial.println("POST /audio/test");
    startAudioPlayback();
    g_server.send(200, "application/json", "{\"ok\":true}");
  });

  g_server.on("/audio/enable", HTTP_POST, []() {
    Serial.println("POST /audio/enable");
    g_audioEnabled = true;
    g_server.send(200, "application/json", "{\"ok\":true}");
  });

  g_server.on("/audio/disable", HTTP_POST, []() {
    Serial.println("POST /audio/disable");
    g_audioEnabled = false;
    stopAudioPlayback();
    g_server.send(200, "application/json", "{\"ok\":true}");
  });

  g_server.on("/audio/config", HTTP_POST, []() {
    Serial.println("POST /audio/config");
    if (g_server.hasArg("mode")) {
      g_audioMode = g_server.arg("mode");
    }
    if (g_server.hasArg("pattern")) {
      g_audioPatternId = g_server.arg("pattern");
    }
    g_server.send(200, "application/json", "{\"ok\":true}");
  });

  g_server.begin();
  Serial.println("Web server started on port 80");
}

void setupImpl() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("XIAO minimal camera test");
  Serial.printf("LED_BUILTIN=%d (active low)\n", LED_BUILTIN);

  // 1. Init LED first so we know it's alive
  pinMode(LED_BUILTIN, OUTPUT);
  setBuiltinLed(true); // Turn ON during boot

  // Init buzzer pin
  ledcAttachChannel(kBuzzerPin, 2000, 8, kBuzzerChannel);
  ledcWriteTone(kBuzzerPin, 0);
  ledcWrite(kBuzzerPin, 0);

#ifdef ATTRAX_ROUND_DISPLAY_PORT
  // Bring the screen up first so boot status is visible even if later subsystems fail.
  Serial.println("Initializing Round Display LVGL...");
  lv_init();
#if LVGL_VERSION_MAJOR == 9
  lv_tick_set_cb(millis);
#endif
  screen_rotation = 0;
  lv_xiao_disp_init();
  lv_xiao_touch_init();
  createRoundUi();
  ledcAttachChannel(kDisplayBacklightPin, 5000, 8, kDisplayBacklightChannel);
  // Keep the Lite build steadily visible; the breathing effect makes the backlight
  // appear black for long stretches on the round display.
  g_oledBreathingCycleMs = 0;
  g_autoProximityOled = false;
  setDisplayBacklight(255);
  g_oledReady = true;
  updateOled();
  lv_timer_handler();
  Serial.println("Round Display LVGL initialized.");
#endif

  // 2. Init WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  // Explicitly configure AP IP to 192.168.4.1
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  // Allow up to 4 devices, sometimes phones open multiple connections or mac+phone connects
  const bool apOk = WiFi.softAP(kApName, nullptr, 1, 0, 4);
  g_apStatus = apOk ? "started" : "failed";
  Serial.printf("AP %s: %s (%s)\n", apOk ? "started" : "failed", kApName, ipToString(WiFi.softAPIP()).c_str());

  // 3. Init Camera early
#ifdef ATTRAX_ROUND_DISPLAY_PORT
  g_cameraReady = false;
  g_cameraError = "OFF";
  Serial.println("Round port: camera disabled due to GPIO10 conflict with display MOSI.");
#else
  ensureCameraReady();
#endif

  // 3.5 Init SD Card
#ifdef ATTRAX_ROUND_DISPLAY_PORT
  g_sdCardMounted = false;
  Serial.println("Round port: SD disabled for stable bring-up.");
#else
  initSD();
#endif

  // 4. Init Mic
  setupMic();

  // 5. Init Web Server
  setupWebServer();

  // 5.5 Init BLE
#ifdef ATTRAX_ROUND_DISPLAY_PORT
  Serial.println("Round port: BLE disabled for stable bring-up.");
#else
  setupBLE();
#endif

  // 6. Init soil sensor and DHT20
#ifndef ATTRAX_ROUND_DISPLAY_PORT
  analogSetPinAttenuation(kSoilMoisturePin, ADC_11db);
  pinMode(kUltrasonicSigPinTx, INPUT);
  pinMode(kUltrasonicSigPinRx, INPUT);
#endif
  pollSensors();

  // 7. Init OLED safely
#ifdef ATTRAX_ROUND_DISPLAY_PORT
  updateOled();
#else
  Serial.println("Initializing OLED via Software I2C...");
  // Initialize U8g2 using its built-in Software I2C (bypasses Wire.h completely)
  g_u8g2.begin();
  // Clear the screen explicitly to black
  g_u8g2.clearDisplay();
  g_u8g2.setPowerSave(0); // Wake up the display
  g_u8g2.setContrast(255); // Max contrast
  
  g_oledReady = true;
  updateOled();
  Serial.println("OLED initialized.");
#endif

  // Boot done, turn off LED
  setBuiltinLed(false);
}

// Helper function to send buffer directly over BLE (bypassing SD card)
void sendBufferOverBle(const String& filename, const uint8_t* data, size_t size) {
  if (!g_bleConnected) return;
  Serial.printf("Sending buffer directly: %s (size: %u)\n", filename.c_str(), size);

  // Send Header
  String header = "FILE:" + filename + ":" + String(size);
  g_bleFileDataChar->setValue((uint8_t*)header.c_str(), header.length());
  g_bleFileDataChar->notify();
  delay(100);

  // Send Chunks
  size_t offset = 0;
  while (offset < size && g_bleConnected) {
    size_t chunkLen = (size - offset > 200) ? 200 : (size - offset);
    g_bleFileDataChar->setValue((uint8_t*)(data + offset), chunkLen);
    g_bleFileDataChar->notify();
    offset += chunkLen;
    delay(15); // Prevent BLE buffer overflow
  }

  // Send EOF
  if (g_bleConnected) {
    String eof = "EOF:" + filename;
    g_bleFileDataChar->setValue((uint8_t*)eof.c_str(), eof.length());
    g_bleFileDataChar->notify();
    delay(100);
    
    // Signal UI that transfer is done
    String done = "SYNC_DONE";
    g_bleFileDataChar->setValue((uint8_t*)done.c_str(), done.length());
    g_bleFileDataChar->notify();
  }
  Serial.println("Direct BLE transfer complete.");
}

void updateProximityOled() {
  if (!g_autoProximityOled || !g_oledReady) return;
  if (isnan(g_distanceCm)) return;

  unsigned long now = millis();
  if (now - g_lastProximityUpdateMs >= 200) {
    g_lastProximityUpdateMs = now;

    // 静态变量用于“人一直在”（停留）的检测
    static float lastDist = -1;
    static unsigned long dwellStartTime = 0;
    static bool isDwelling = false;

    if (g_distanceCm < 150.0) {
      g_lastPersonDetectedMs = now;
    }

    // 场景1：人离开超过 10 秒，回到信息显示页面 (Mode 0)
    if (now - g_lastPersonDetectedMs > 10000) {
      if (g_oledDisplayMode != 0) {
        g_oledDisplayMode = 0;
        g_oledBreathingCycleMs = 0; // 停止呼吸效果
        g_lastOledRefreshMs = 0;
      }
      isDwelling = false;
      lastDist = -1;
    } else {
      // 人在视野内
      uint8_t newMode = g_oledDisplayMode;
      unsigned long newSpeed = g_oledBreathingCycleMs;

      // 停留检测：如果距离变化大于 15cm，认为人在移动，重置停留时间
      if (lastDist < 0 || abs(g_distanceCm - lastDist) > 15.0) {
        dwellStartTime = now;
        lastDist = g_distanceCm;
        isDwelling = false;
      } else if (now - dwellStartTime > 4000) {
        // 如果距离在 15cm 范围内稳定超过 4 秒，进入“停留”状态
        isDwelling = true;
      }

      if (isDwelling) {
        // 场景4：人一直在 (停留)
        // 保持当前的呼吸模式（根据距离判断半亮或全白），但将呼吸放缓，避免持续剧烈闪烁打扰用户
        newMode = (g_distanceCm < 80.0) ? 3 : 2; 
        newSpeed = 4000; // 平缓深长的呼吸 (4秒)
      } else {
        // 动态互动
        if (g_distanceCm > 100.0) {
          // 场景2：刚感应到人 (100 - 150cm)
          newMode = 2; // 半屏纯白（类似刚睁开眼或注意到你）
          newSpeed = map((long)g_distanceCm, 100, 150, 2000, 4000); // 距离越远越平缓
        } else if (g_distanceCm > 40.0) {
          // 场景3：人靠近 (40 - 100cm)
          newMode = 3; // 全屏纯白（完全聚焦）
          newSpeed = map((long)g_distanceCm, 40, 100, 800, 2000); // 距离越近，呼吸从 2秒 加快到 0.8秒，像心跳加速
        } else {
          // 场景3进阶：人非常近 (< 40cm)
          newMode = 3; 
          newSpeed = map((long)g_distanceCm, 0, 40, 300, 800); // 极度急促的呼吸，兴奋状态
        }
      }

      newSpeed = constrain(newSpeed, 300, 4000);

      // 如果模式发生变化，或呼吸速度发生较大变化(>200ms)才下发更新，避免频繁刷新
      if (newMode != g_oledDisplayMode || abs((long)newSpeed - (long)g_oledBreathingCycleMs) > 200) {
        g_oledDisplayMode = newMode;
        g_oledBreathingCycleMs = newSpeed;
        g_lastOledRefreshMs = 0; // force redraw
      }
    }
  }
}

void loopImpl() {
#ifdef ATTRAX_ROUND_DISPLAY_PORT
  lv_timer_handler();
  updatePartnerHeartEffect();
  if (smartPlantConsumeBacklightRequest()) startHeartBacklight();
#endif
  g_server.handleClient();
  pollMic();
  pollSensors();
  updateProximityOled();
  updateOled();
  updateOledBreathing();
  updateAudioEngine();
  notifyBleSensors();

  if (g_bleShouldAdvertise && (millis() - g_bleDisconnectTime > 500)) {
    g_bleShouldAdvertise = false;
    NimBLEDevice::startAdvertising();
    Serial.println("BLE Advertising restarted safely after disconnect.");
  }

  unsigned long now = millis();
  
  // Check for Proximity Capture (Distance < 50cm)
  if (g_cameraReady && !isnan(g_distanceCm) && g_distanceCm > 0 && g_distanceCm < kProximityThresholdCm) {
    if (now - g_lastProximityCaptureMs > kProximityCaptureCooldownMs) {
      g_lastProximityCaptureMs = now;
      camera_fb_t* fb = captureFrame(false);
      if (fb) {
        String filename = "cam_" + String(now) + ".jpg";
        sendBufferOverBle(filename, fb->buf, fb->len);
        esp_camera_fb_return(fb);
      }
    }
  }

  // Check for Loud Sound Capture
  if (g_micReady && g_micRms > kLoudSoundThreshold) {
    if (now - g_lastLoudSoundTime > kLoudSoundCooldown) {
      g_lastLoudSoundTime = now;
      Serial.println("Loud sound detected! Triggering Audio Record directly to BLE...");
      
      uint32_t sampleRate = kMicSampleRate;
      uint16_t numChannels = 1;
      uint16_t bitsPerSample = 16;
      uint32_t durationSec = 3;
      uint32_t numSamples = sampleRate * durationSec;
      size_t audioSize = numSamples * (bitsPerSample / 8) * numChannels;
      
      uint8_t* audioBuf = (uint8_t*)heap_caps_malloc(audioSize, MALLOC_CAP_SPIRAM);
      if (!audioBuf) audioBuf = (uint8_t*)malloc(audioSize); // Fallback
      
      if (audioBuf) {
        uint32_t samplesWritten = 0;
        size_t bytesRead = 0;
        int16_t rawBuf[1024];
        size_t offset = 0;
        while (samplesWritten < numSamples && g_bleConnected) {
          esp_err_t err = i2s_read(I2S_NUM_0, rawBuf, sizeof(rawBuf), &bytesRead, 20 / portTICK_PERIOD_MS);
          if (err == ESP_OK && bytesRead > 0) {
            for (size_t i = 0; i < bytesRead / 2; ++i) {
               rawBuf[i] = (int16_t)((uint16_t)rawBuf[i] << 2);
            }
            size_t toCopy = bytesRead;
            if (offset + toCopy > audioSize) toCopy = audioSize - offset;
            memcpy(audioBuf + offset, rawBuf, toCopy);
            offset += toCopy;
            samplesWritten += (toCopy / 2);
          }
          delay(1);
        }
        String filename = "mic_base64_" + String(now) + ".raw";
        sendBufferOverBle(filename, audioBuf, offset);
        free(audioBuf);
      } else {
        Serial.println("Failed to allocate memory for loud sound audio.");
      }
    }
  }

  // Handle BLE Triggered Capture
  if (g_startBleCapture && g_bleConnected) {
    g_startBleCapture = false;
    Serial.println("BLE Command: Triggering Capture directly to BLE...");
    if (g_cameraReady) {
      camera_fb_t* fb = captureFrame(false);
      if (fb) {
        String filename = "cam_ble_" + String(now) + ".jpg";
        sendBufferOverBle(filename, fb->buf, fb->len);
        esp_camera_fb_return(fb);
      } else {
        String err = "ERR:Camera capture failed";
        g_bleFileDataChar->setValue((uint8_t*)err.c_str(), err.length());
        g_bleFileDataChar->notify();
      }
    } else {
      Serial.println("Cannot capture: Camera not ready.");
      String err = "ERR:Camera not ready";
      g_bleFileDataChar->setValue((uint8_t*)err.c_str(), err.length());
      g_bleFileDataChar->notify();
    }
  }

  // Handle BLE Triggered Record
  if (g_startBleRecord && g_bleConnected) {
    g_startBleRecord = false;
    Serial.println("BLE Command: Triggering Audio Record directly to BLE...");
    if (g_micReady) {
      uint32_t sampleRate = kMicSampleRate;
      uint16_t numChannels = 1;
      uint16_t bitsPerSample = 16;
      uint32_t durationSec = 3;
      uint32_t numSamples = sampleRate * durationSec;
      uint32_t dataSize = numSamples * numChannels * (bitsPerSample / 8);
      uint32_t fileSize = 36 + dataSize;
      
      uint8_t* audioBuf = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM);
      if (!audioBuf) audioBuf = (uint8_t*)malloc(fileSize); // Fallback
      
      if (audioBuf) {
        memcpy(audioBuf, "RIFF", 4);
        memcpy(audioBuf + 4, &fileSize, 4);
        memcpy(audioBuf + 8, "WAVE", 4);
        memcpy(audioBuf + 12, "fmt ", 4);
        uint32_t fmtSize = 16;
        memcpy(audioBuf + 16, &fmtSize, 4);
        uint16_t audioFormat = 1;
        memcpy(audioBuf + 20, &audioFormat, 2);
        memcpy(audioBuf + 22, &numChannels, 2);
        memcpy(audioBuf + 24, &sampleRate, 4);
        uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
        memcpy(audioBuf + 28, &byteRate, 4);
        uint16_t blockAlign = numChannels * (bitsPerSample / 8);
        memcpy(audioBuf + 32, &blockAlign, 2);
        memcpy(audioBuf + 34, &bitsPerSample, 2);
        memcpy(audioBuf + 36, "data", 4);
        memcpy(audioBuf + 40, &dataSize, 4);

        uint32_t samplesWritten = 0;
        size_t bytesRead = 0;
        int16_t rawBuf[1024];
        size_t offset = 44;
        while (samplesWritten < numSamples && g_bleConnected) {
          esp_err_t err = i2s_read(I2S_NUM_0, rawBuf, sizeof(rawBuf), &bytesRead, 20 / portTICK_PERIOD_MS);
          if (err == ESP_OK && bytesRead > 0) {
            for (size_t i = 0; i < bytesRead / 2; ++i) {
               rawBuf[i] = (int16_t)((uint16_t)rawBuf[i] << 2);
            }
            size_t toCopy = bytesRead;
            if (offset + toCopy > fileSize) toCopy = fileSize - offset;
            memcpy(audioBuf + offset, rawBuf, toCopy);
            offset += toCopy;
            samplesWritten += (toCopy / 2);
          }
          delay(1);
        }
        String filename = "mic_ble_" + String(now) + ".wav";
        sendBufferOverBle(filename, audioBuf, offset);
        free(audioBuf);
      } else {
        Serial.println("Failed to allocate memory for BLE audio record.");
        String err = "ERR:Out of memory";
        g_bleFileDataChar->setValue((uint8_t*)err.c_str(), err.length());
        g_bleFileDataChar->notify();
      }
    } else {
      Serial.println("Cannot record: Mic not ready.");
      String err = "ERR:Mic not ready";
      g_bleFileDataChar->setValue((uint8_t*)err.c_str(), err.length());
      g_bleFileDataChar->notify();
    }
  }

  // Handle BLE SD Card Sync
  if (g_startBleSync && g_bleConnected) {
    g_startBleSync = false;
    Serial.println("Starting BLE Sync of SD captures...");
    File dir = SD.open("/captures");
    if (dir && dir.isDirectory()) {
      File file = dir.openNextFile();
      while (file && g_bleConnected) {
        if (!file.isDirectory()) {
          String fname = file.name();
          int slash = fname.lastIndexOf('/');
          if (slash >= 0) fname = fname.substring(slash + 1);

          size_t fileSize = file.size();
          Serial.printf("Sending file: %s (size: %u)\n", fname.c_str(), fileSize);

          // Send Header
          String header = "FILE:" + fname + ":" + String(fileSize);
          g_bleFileDataChar->setValue((uint8_t*)header.c_str(), header.length());
          g_bleFileDataChar->notify();
          delay(100); // Give client time to process header

          // Send Data in Chunks
          uint8_t buf[200];
          while (file.available() && g_bleConnected) {
            size_t len = file.read(buf, sizeof(buf));
            g_bleFileDataChar->setValue(buf, len);
            g_bleFileDataChar->notify();
            delay(15); // Yield / prevent BLE buffer overflow
          }

          // Send EOF
          String eof = "EOF:" + fname;
          g_bleFileDataChar->setValue((uint8_t*)eof.c_str(), eof.length());
          g_bleFileDataChar->notify();
          delay(100);
          
          file.close();
          // Delete file after sync
          SD.remove(String("/captures/") + fname);
          Serial.println("Deleted synced file: " + fname);
        }
        file = dir.openNextFile();
      }
      String done = "SYNC_DONE";
      g_bleFileDataChar->setValue((uint8_t*)done.c_str(), done.length());
      g_bleFileDataChar->notify();
      Serial.println("BLE Sync complete.");
    } else {
      String done = "SYNC_DONE";
      g_bleFileDataChar->setValue((uint8_t*)done.c_str(), done.length());
      g_bleFileDataChar->notify();
    }
  }
}

}  // namespace

void setup() {
  setupImpl();
}

void loop() {
  loopImpl();
}
#endif
