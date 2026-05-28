#include "smart_plant_app.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr uint32_t kBg = 0x07130f;
constexpr uint32_t kPanel = 0x10241d;
constexpr uint32_t kCard = 0x153128;
constexpr uint32_t kSelected = 0x254f40;
constexpr uint32_t kGreen = 0x4de395;
constexpr uint32_t kMint = 0xa7f6c8;
constexpr uint32_t kSubtext = 0x78a796;
constexpr uint32_t kText = 0xe5faee;
constexpr uint32_t kBlue = 0x63c9ed;
constexpr uint32_t kSun = 0xf3c45b;
constexpr uint32_t kRed = 0xff6685;
constexpr uint32_t kSpriteBody = 0x091b16;
constexpr uint32_t kSpriteEye = 0xf3fff7;
constexpr uint8_t kHistorySize = 8;
constexpr uint8_t kEventSize = 5;
constexpr uint32_t kSampleIntervalMs = 30000;
constexpr uint32_t kDemoStageMs = 3000;
constexpr uint32_t kIdleReturnMs = 45000;
constexpr uint32_t kCuriousAfterMs = 18000;
constexpr uint32_t kCuriousDurationMs = 5500;

enum class Page : uint8_t {
  kHome,
  kPlants,
  kTrend,
  kCare,
};

enum class Mood : uint8_t {
  kNormal,
  kHappy,
  kSad,
  kCurious,
  kLove,
};

struct Plant {
  const char* name;
  const char* kind;
  uint32_t leafColor;
  int tempMin;
  int tempMax;
  int humidityMin;
  int humidityMax;
  int soilMin;
  int soilMax;
};

struct Reading {
  uint32_t minute;
  int soil;
  float temperature;
  float humidity;
};

struct UiState {
  lv_obj_t* root = nullptr;
  lv_obj_t* content = nullptr;
  lv_obj_t* chrome = nullptr;
  lv_obj_t* toast = nullptr;
  lv_obj_t* idleHint = nullptr;
  lv_obj_t* homeScore = nullptr;
  lv_obj_t* homeReading = nullptr;
  lv_obj_t* nav[3] = {nullptr, nullptr, nullptr};
  lv_obj_t* navText[3] = {nullptr, nullptr, nullptr};
  Page page = Page::kHome;
  uint8_t selectedPlant = 0;
  int soilPercent = -1;
  float temperatureC = NAN;
  float humidityPct = NAN;
  uint8_t phonesOnline = 0;
  Mood mood = Mood::kNormal;
  Reading history[kHistorySize] = {};
  uint8_t historyCount = 0;
  uint8_t historyNext = 0;
  uint32_t lastSampleMs = 0;
  uint32_t loveUntilMs = 0;
  uint32_t temporaryMoodUntilMs = 0;
  uint32_t lastInteractionMs = 0;
  uint32_t idleSinceMs = 0;
  uint32_t demoStartedMs = 0;
  uint8_t demoStage = 0;
  bool demoActive = false;
  bool idleMode = true;
  bool requestBacklight = false;
  char events[kEventSize][34] = {};
  uint8_t eventCount = 0;
};

const Plant kPlants[] = {
    {"Mossy", "MONSTERA", 0x51df8c, 18, 30, 55, 80, 45, 70},
    {"Minto", "MINT", 0x71ecad, 15, 25, 40, 70, 55, 80},
    {"Sunny", "SUCCULENT", 0xb4dd62, 18, 30, 30, 55, 15, 35},
    {"Lily", "PEACE LILY", 0x45c879, 18, 29, 50, 75, 55, 80},
    {"Pothos", "GOLDEN POTHOS", 0x8bd45b, 17, 30, 40, 70, 35, 65},
};
constexpr uint8_t kPlantCount = sizeof(kPlants) / sizeof(kPlants[0]);
UiState ui;

lv_color_t color(uint32_t hex) {
  return lv_color_hex(hex);
}

void resetObject(lv_obj_t* obj) {
  lv_obj_remove_style_all(obj);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

lv_obj_t* panel(lv_obj_t* parent, int x, int y, int width, int height, int radius,
                uint32_t fill) {
  lv_obj_t* obj = lv_obj_create(parent);
  resetObject(obj);
  lv_obj_set_pos(obj, x, y);
  lv_obj_set_size(obj, width, height);
  lv_obj_set_style_bg_color(obj, color(fill), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(obj, radius, 0);
  return obj;
}

lv_obj_t* label(lv_obj_t* parent, const char* text, lv_color_t textColor,
                const lv_font_t* font = &lv_font_montserrat_10) {
  lv_obj_t* obj = lv_label_create(parent);
  lv_label_set_text(obj, text);
  lv_obj_set_style_text_font(obj, font, 0);
  lv_obj_set_style_text_color(obj, textColor, 0);
  return obj;
}

void centered(lv_obj_t* obj) {
  lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(obj);
}

void addEvent(const char* text) {
  for (int i = kEventSize - 1; i > 0; i--) {
    snprintf(ui.events[i], sizeof(ui.events[i]), "%s", ui.events[i - 1]);
  }
  snprintf(ui.events[0], sizeof(ui.events[0]), "%s", text);
  if (ui.eventCount < kEventSize) ui.eventCount++;
}

int rangeScore(float reading, int minimum, int maximum) {
  if (isnan(reading)) return -1;
  if (reading >= minimum && reading <= maximum) return 100;
  const float distance = reading < minimum ? minimum - reading : reading - maximum;
  return max(20, 100 - static_cast<int>(distance * 7.0f));
}

int assessmentScore() {
  const Plant& plant = kPlants[ui.selectedPlant];
  int total = 0;
  int count = 0;
  const int temperature = rangeScore(ui.temperatureC, plant.tempMin, plant.tempMax);
  const int humidity = rangeScore(ui.humidityPct, plant.humidityMin, plant.humidityMax);
  const int soil = ui.soilPercent < 0 ? -1 : rangeScore(ui.soilPercent, plant.soilMin, plant.soilMax);
  if (temperature >= 0) {
    total += temperature;
    count++;
  }
  if (humidity >= 0) {
    total += humidity;
    count++;
  }
  if (soil >= 0) {
    total += soil;
    count++;
  }
  return count == 0 ? -1 : total / count;
}

const char* scoreText(int score) {
  if (score < 0) return "WAIT SENSOR";
  if (score >= 90) return "THRIVING";
  if (score >= 72) return "GOOD";
  if (score >= 50) return "ADJUST";
  return "ATTENTION";
}

uint32_t scoreColor(int score) {
  if (score >= 72) return kGreen;
  if (score >= 50) return kSun;
  return kRed;
}

Mood assessedMood() {
  const int score = assessmentScore();
  if (score < 0) return Mood::kCurious;
  if (score >= 90) return Mood::kHappy;
  if (score < 55) return Mood::kSad;
  return Mood::kNormal;
}

void drawLeaf(lv_obj_t* parent, int x, int y, int width, int height, uint32_t fill) {
  panel(parent, x, y, width, height, LV_RADIUS_CIRCLE, fill);
}

const char* moodText(Mood mood) {
  switch (mood) {
    case Mood::kHappy:
      return "HAPPY";
    case Mood::kSad:
      return "NEEDS CARE";
    case Mood::kCurious:
      return "CURIOUS";
    case Mood::kLove:
      return "LOVED";
    case Mood::kNormal:
    default:
      return "CALM";
  }
}

void drawSpriteEye(lv_obj_t* parent, int x, int y, int width, int height) {
  panel(parent, x, y, width, height, height / 2, kSpriteEye);
}

void drawSpriteAt(lv_obj_t* parent, Mood mood, int x, int y, int width, int height,
                  bool showState) {
  lv_obj_t* sprite = panel(parent, x, y, width, height, LV_RADIUS_CIRCLE, kSpriteBody);
  const float sx = width / 86.0f;
  const float sy = height / 66.0f;
  auto px = [sx](int value) { return static_cast<int>(roundf(value * sx)); };
  auto py = [sy](int value) { return static_cast<int>(roundf(value * sy)); };
  drawLeaf(sprite, px(31), -py(5), px(17), py(9), kGreen);
  drawLeaf(sprite, px(46), -py(10), px(22), py(10), kMint);
  switch (mood) {
    case Mood::kHappy:
      drawSpriteEye(sprite, px(22), py(27), px(13), py(8));
      drawSpriteEye(sprite, px(51), py(27), px(13), py(8));
      panel(sprite, px(32), py(45), px(22), py(5), LV_RADIUS_CIRCLE, kGreen);
      break;
    case Mood::kSad:
      drawSpriteEye(sprite, px(22), py(23), px(12), py(22));
      drawSpriteEye(sprite, px(52), py(23), px(12), py(22));
      panel(sprite, px(36), py(53), px(14), py(4), LV_RADIUS_CIRCLE, kBlue);
      break;
    case Mood::kCurious:
      drawSpriteEye(sprite, px(19), py(18), px(13), py(29));
      drawSpriteEye(sprite, px(50), py(23), px(18), py(24));
      panel(sprite, px(38), py(49), px(8), py(8), LV_RADIUS_CIRCLE, kSpriteEye);
      break;
    case Mood::kLove:
      drawSpriteEye(sprite, px(17), py(20), px(18), py(25));
      drawSpriteEye(sprite, px(51), py(20), px(18), py(25));
      lv_obj_set_pos(label(sprite, "<3", color(kRed), &lv_font_montserrat_14), px(34), py(45));
      break;
    case Mood::kNormal:
    default:
      drawSpriteEye(sprite, px(21), py(18), px(13), py(30));
      drawSpriteEye(sprite, px(52), py(18), px(13), py(30));
      break;
  }
  if (showState) {
    lv_obj_t* state = label(parent, moodText(mood), color(mood == Mood::kLove ? kRed : kGreen));
    lv_obj_set_pos(state, 166, 56);
    lv_obj_set_width(state, 64);
    lv_label_set_long_mode(state, LV_LABEL_LONG_DOT);
  }
}

void drawSprite(lv_obj_t* parent, Mood mood) {
  drawSpriteAt(parent, mood, 17, 34, 72, 55, true);
}

void updateNav() {
  for (uint8_t i = 0; i < 3; i++) {
    const Page navPage = i == 0 ? Page::kHome : (i == 1 ? Page::kTrend : Page::kCare);
    const bool active = ui.page == navPage;
    lv_obj_set_style_bg_opa(ui.nav[i], active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(ui.navText[i], color(active ? kGreen : kSubtext), 0);
  }
}

void renderPage();
void renderIdle();

void wakeApp(Page page = Page::kHome) {
  ui.idleMode = false;
  ui.page = page;
  ui.lastInteractionMs = millis();
  if (ui.chrome) lv_obj_clear_flag(ui.chrome, LV_OBJ_FLAG_HIDDEN);
  renderPage();
}

void enterIdle() {
  ui.idleMode = true;
  ui.idleSinceMs = millis();
  if (ui.chrome) lv_obj_add_flag(ui.chrome, LV_OBJ_FLAG_HIDDEN);
  renderIdle();
}

void onRootClicked(lv_event_t* event) {
  (void)event;
  if (ui.idleMode) {
    wakeApp(Page::kHome);
  } else {
    ui.lastInteractionMs = millis();
  }
}

void beginDemo() {
  ui.demoActive = true;
  ui.demoStartedMs = millis();
  ui.demoStage = 255;
  wakeApp(Page::kHome);
  addEvent("Demo mode started");
}

void onNavClicked(lv_event_t* event) {
  ui.page = static_cast<Page>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
  ui.lastInteractionMs = millis();
  renderPage();
}

void updateHomeLabels() {
  if (!ui.homeScore || !ui.homeReading) return;
  const int score = assessmentScore();
  char scoreValue[30];
  if (score < 0) snprintf(scoreValue, sizeof(scoreValue), "--  %s", scoreText(score));
  else snprintf(scoreValue, sizeof(scoreValue), "%d  %s", score, scoreText(score));
  lv_label_set_text(ui.homeScore, scoreValue);
  lv_obj_set_style_text_color(ui.homeScore, color(scoreColor(score)), 0);

  char soil[8];
  char humidity[8];
  char reading[42];
  if (ui.soilPercent < 0) snprintf(soil, sizeof(soil), "S--");
  else snprintf(soil, sizeof(soil), "S%d%%", ui.soilPercent);
  if (isnan(ui.humidityPct)) snprintf(humidity, sizeof(humidity), "H--");
  else snprintf(humidity, sizeof(humidity), "H%.0f%%", ui.humidityPct);
  if (isnan(ui.temperatureC)) snprintf(reading, sizeof(reading), "%s  %s  T--", soil, humidity);
  else snprintf(reading, sizeof(reading), "%s  %s  T%.0fC", soil, humidity, ui.temperatureC);
  lv_label_set_text(ui.homeReading, reading);
}

void renderHome() {
  const Plant& plant = kPlants[ui.selectedPlant];

  lv_obj_t* eyebrow = label(ui.content, "NOW GROWING", color(kSubtext));
  lv_obj_set_pos(eyebrow, 64, 4);
  lv_obj_t* name = label(ui.content, plant.name, color(kText), &lv_font_montserrat_16);
  lv_obj_set_pos(name, 64, 18);
  lv_obj_t* kind = label(ui.content, plant.kind, color(kMint));
  lv_obj_set_pos(kind, 64, 38);

  drawSprite(ui.content, ui.mood);

  lv_obj_t* evaluation = panel(ui.content, 30, 94, 180, 28, 14, kCard);
  lv_obj_set_pos(label(evaluation, "ENV", color(kSubtext)), 13, 10);
  ui.homeScore = label(evaluation, "--", color(kGreen), &lv_font_montserrat_12);
  lv_obj_align(ui.homeScore, LV_ALIGN_RIGHT_MID, -12, 0);
  ui.homeReading = label(ui.content, "--", color(kText), &lv_font_montserrat_12);
  lv_obj_align(ui.homeReading, LV_ALIGN_BOTTOM_MID, 0, -6);
  updateHomeLabels();
}

void onPlantClicked(lv_event_t* event) {
  ui.selectedPlant = static_cast<uint8_t>(
      reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
  char eventText[34];
  snprintf(eventText, sizeof(eventText), "You chose %s", kPlants[ui.selectedPlant].name);
  addEvent(eventText);
  ui.mood = Mood::kCurious;
  ui.temporaryMoodUntilMs = millis() + 5000;
  ui.lastInteractionMs = millis();
  ui.page = Page::kHome;
  renderPage();
}

void renderPlants() {
  lv_obj_set_pos(label(ui.content, "Choose plant", color(kText), &lv_font_montserrat_14), 41, 0);
  for (uint8_t i = 0; i < kPlantCount; i++) {
    const Plant& plantItem = kPlants[i];
    lv_obj_t* row = panel(ui.content, 25, 23 + i * 24, 190, 21, 10,
                          i == ui.selectedPlant ? kSelected : kCard);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, onPlantClicked, LV_EVENT_CLICKED,
                        reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
    panel(row, 10, 7, 8, 8, LV_RADIUS_CIRCLE, plantItem.leafColor);
    lv_obj_set_pos(label(row, plantItem.name, color(kText)), 27, 5);
    lv_obj_t* action = label(row, i == ui.selectedPlant ? "NOW GROWING" : "SELECT",
                             color(i == ui.selectedPlant ? kGreen : kSubtext));
    lv_obj_align(action, LV_ALIGN_RIGHT_MID, -10, 0);
  }
}

void renderTrend() {
  const int score = assessmentScore();
  char line[44];
  lv_obj_set_pos(label(ui.content, "Growth history", color(kText), &lv_font_montserrat_14), 37, 0);
  snprintf(line, sizeof(line), "%s  ENV %s", kPlants[ui.selectedPlant].name, scoreText(score));
  lv_obj_set_pos(label(ui.content, line, color(scoreColor(score))), 37, 19);

  lv_obj_t* table = panel(ui.content, 25, 38, 190, 100, 12, kCard);
  if (ui.historyCount == 0) {
    lv_obj_t* empty = label(table, "Collecting readings...", color(kSubtext));
    lv_obj_center(empty);
    return;
  }
  const uint8_t visible = min(static_cast<uint8_t>(5), ui.historyCount);
  for (uint8_t i = 0; i < visible; i++) {
    const uint8_t index = (ui.historyNext + kHistorySize - 1 - i) % kHistorySize;
    const Reading& reading = ui.history[index];
    char soil[6];
    if (reading.soil < 0) snprintf(soil, sizeof(soil), "--");
    else snprintf(soil, sizeof(soil), "%d", reading.soil);
    snprintf(line, sizeof(line), "-%02lum  T%2.0f  H%2.0f  S%s",
             static_cast<unsigned long>((millis() / 60000) - reading.minute),
             reading.temperature, reading.humidity, soil);
    lv_obj_set_pos(label(table, line, color(i == 0 ? kMint : kSubtext)), 11, 8 + i * 18);
  }
}

void onCareClicked(lv_event_t* event) {
  (void)event;
  addEvent("You | watered today");
  ui.mood = Mood::kHappy;
  ui.temporaryMoodUntilMs = millis() + 5000;
  ui.lastInteractionMs = millis();
  renderPage();
}

void onDemoClicked(lv_event_t* event) {
  (void)event;
  beginDemo();
}

void renderCare() {
  char line[38];
  lv_obj_set_pos(label(ui.content, "Care together", color(kText), &lv_font_montserrat_14), 38, 0);
  snprintf(line, sizeof(line), "%u phone%s online | hearts ready", ui.phonesOnline,
           ui.phonesOnline == 1 ? "" : "s");
  lv_obj_set_pos(label(ui.content, line, color(kSubtext)), 38, 18);

  lv_obj_t* events = panel(ui.content, 25, 38, 190, 73, 12, kCard);
  for (uint8_t i = 0; i < min(static_cast<uint8_t>(3), ui.eventCount); i++) {
    lv_obj_set_pos(label(events, ui.events[i], color(i == 0 ? kMint : kSubtext)), 11, 9 + i * 20);
  }

  lv_obj_t* care = panel(ui.content, 25, 118, 111, 28, 14, kGreen);
  lv_obj_add_flag(care, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(care, onCareClicked, LV_EVENT_CLICKED, nullptr);
  centered(label(care, "LOG WATERING", color(kBg), &lv_font_montserrat_12));
  lv_obj_t* demo = panel(ui.content, 142, 118, 73, 28, 14, kSelected);
  lv_obj_add_flag(demo, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(demo, onDemoClicked, LV_EVENT_CLICKED, nullptr);
  centered(label(demo, "DEMO", color(kGreen), &lv_font_montserrat_12));
}

void renderPage() {
  if (!ui.content) return;
  if (ui.idleMode) {
    renderIdle();
    return;
  }
  ui.homeScore = nullptr;
  ui.homeReading = nullptr;
  lv_obj_clean(ui.content);
  updateNav();
  switch (ui.page) {
    case Page::kPlants:
      renderPlants();
      break;
    case Page::kTrend:
      renderTrend();
      break;
    case Page::kCare:
      renderCare();
      break;
    case Page::kHome:
    default:
      renderHome();
      break;
  }
}

void renderIdle() {
  if (!ui.content) return;
  ui.homeScore = nullptr;
  ui.homeReading = nullptr;
  lv_obj_clean(ui.content);
  drawSpriteAt(ui.content, ui.mood, 38, 43, 164, 126, false);
  lv_obj_t* mood = label(ui.content, moodText(ui.mood),
                         color(ui.mood == Mood::kLove ? kRed : kGreen),
                         &lv_font_montserrat_14);
  lv_obj_align(mood, LV_ALIGN_BOTTOM_MID, 0, -39);
  ui.idleHint = label(ui.content, "tap to wake", color(kSubtext));
  lv_obj_align(ui.idleHint, LV_ALIGN_BOTTOM_MID, 0, -23);
}

void hideToast(lv_timer_t* timer) {
  (void)timer;
  if (ui.toast) lv_obj_add_flag(ui.toast, LV_OBJ_FLAG_HIDDEN);
}

void saveReading() {
  Reading& reading = ui.history[ui.historyNext];
  reading.minute = millis() / 60000;
  reading.soil = ui.soilPercent;
  reading.temperature = ui.temperatureC;
  reading.humidity = ui.humidityPct;
  ui.historyNext = (ui.historyNext + 1) % kHistorySize;
  if (ui.historyCount < kHistorySize) ui.historyCount++;
  ui.lastSampleMs = millis();
}

void updateDemo() {
  if (!ui.demoActive) return;
  const uint8_t stage = static_cast<uint8_t>((millis() - ui.demoStartedMs) / kDemoStageMs);
  if (stage == ui.demoStage) return;
  ui.demoStage = stage;
  switch (stage) {
    case 0:
      ui.selectedPlant = 0;
      ui.page = Page::kHome;
      ui.mood = Mood::kNormal;
      break;
    case 1:
      ui.page = Page::kPlants;
      break;
    case 2:
      ui.selectedPlant = 3;
      ui.page = Page::kHome;
      ui.mood = Mood::kCurious;
      break;
    case 3:
      ui.mood = Mood::kSad;
      break;
    case 4:
      ui.mood = Mood::kHappy;
      break;
    case 5:
      ui.page = Page::kTrend;
      break;
    case 6:
      ui.page = Page::kCare;
      addEvent("Luna | caring together");
      break;
    case 7:
      ui.page = Page::kHome;
      ui.mood = Mood::kLove;
      ui.loveUntilMs = millis() + kDemoStageMs;
      ui.requestBacklight = true;
      addEvent("Luna | sent a heart");
      break;
    default:
      ui.demoActive = false;
      ui.page = Page::kHome;
      ui.mood = Mood::kHappy;
      ui.temporaryMoodUntilMs = millis() + 5000;
      break;
  }
  renderPage();
}

}  // namespace

lv_obj_t* smartPlantCreate(lv_obj_t* parent) {
  ui = UiState{};
  ui.idleMode = true;
  ui.idleSinceMs = millis();
  ui.lastInteractionMs = millis();
  ui.root = lv_obj_create(parent);
  resetObject(ui.root);
  lv_obj_set_size(ui.root, 240, 240);
  lv_obj_set_style_bg_color(ui.root, color(kBg), 0);
  lv_obj_set_style_bg_opa(ui.root, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(ui.root, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_clip_corner(ui.root, true, 0);
  lv_obj_add_flag(ui.root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(ui.root, onRootClicked, LV_EVENT_CLICKED, nullptr);

  ui.content = lv_obj_create(ui.root);
  resetObject(ui.content);
  lv_obj_set_pos(ui.content, 0, 0);
  lv_obj_set_size(ui.content, 240, 182);

  ui.chrome = lv_obj_create(ui.root);
  resetObject(ui.chrome);
  lv_obj_set_size(ui.chrome, 240, 240);
  lv_obj_set_pos(ui.chrome, 0, 0);
  panel(ui.chrome, 70, 17, 5, 5, LV_RADIUS_CIRCLE, kGreen);
  lv_obj_t* brand = label(ui.chrome, "SMART PLANT", color(kGreen));
  lv_obj_align(brand, LV_ALIGN_TOP_MID, 0, 11);
  lv_obj_t* plantPicker = panel(ui.chrome, 72, 151, 96, 20, 10, kSelected);
  lv_obj_add_flag(plantPicker, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(plantPicker, onNavClicked, LV_EVENT_CLICKED,
                      reinterpret_cast<void*>(static_cast<uintptr_t>(Page::kPlants)));
  centered(label(plantPicker, "CHANGE PLANT", color(kMint), &lv_font_montserrat_8));

  lv_obj_t* navPanel = panel(ui.chrome, 35, 184, 170, 29, 15, kPanel);
  const char* navLabels[] = {"STATUS", "TREND", "CARE"};
  const Page navPages[] = {Page::kHome, Page::kTrend, Page::kCare};
  for (uint8_t i = 0; i < 3; i++) {
    ui.nav[i] = panel(navPanel, 4 + i * 55, 3, 52, 23, 12, kSelected);
    lv_obj_add_flag(ui.nav[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui.nav[i], onNavClicked, LV_EVENT_CLICKED,
                        reinterpret_cast<void*>(static_cast<uintptr_t>(navPages[i])));
    ui.navText[i] = label(ui.nav[i], navLabels[i], color(kSubtext), &lv_font_montserrat_8);
    centered(ui.navText[i]);
  }
  lv_obj_add_flag(ui.chrome, LV_OBJ_FLAG_HIDDEN);

  ui.toast = panel(ui.root, 42, 73, 156, 46, 17, 0x392332);
  lv_obj_set_style_border_width(ui.toast, 1, 0);
  lv_obj_set_style_border_color(ui.toast, color(kRed), 0);
  centered(label(ui.toast, "<3  LOVE RECEIVED", color(kRed), &lv_font_montserrat_12));
  lv_obj_add_flag(ui.toast, LV_OBJ_FLAG_HIDDEN);

  addEvent("Garden is ready");
  renderIdle();
  return ui.root;
}

void smartPlantUpdate(int soilPercent, float temperatureC, float humidityPct, uint8_t phonesOnline) {
  const uint32_t now = millis();
  const bool temperatureChanged = isnan(temperatureC) != isnan(ui.temperatureC) ||
      (!isnan(temperatureC) && temperatureC != ui.temperatureC);
  const bool humidityChanged = isnan(humidityPct) != isnan(ui.humidityPct) ||
      (!isnan(humidityPct) && humidityPct != ui.humidityPct);
  const bool changed = soilPercent != ui.soilPercent || temperatureChanged ||
      humidityChanged || phonesOnline != ui.phonesOnline;
  ui.soilPercent = soilPercent;
  ui.temperatureC = temperatureC;
  ui.humidityPct = humidityPct;
  ui.phonesOnline = phonesOnline;
  if ((!isnan(ui.temperatureC) || !isnan(ui.humidityPct) || ui.soilPercent >= 0) &&
      (ui.historyCount == 0 || now - ui.lastSampleMs >= kSampleIntervalMs)) {
    saveReading();
  }
  updateDemo();
  if (ui.demoActive) return;

  if (!ui.idleMode && static_cast<long>(now - (ui.lastInteractionMs + kIdleReturnMs)) >= 0) {
    enterIdle();
  }

  const bool temporaryExpired =
      (ui.loveUntilMs && static_cast<long>(now - ui.loveUntilMs) >= 0) ||
      (ui.temporaryMoodUntilMs && static_cast<long>(now - ui.temporaryMoodUntilMs) >= 0);
  if (temporaryExpired) {
    if (ui.temporaryMoodUntilMs && static_cast<long>(now - ui.temporaryMoodUntilMs) >= 0) {
      ui.idleSinceMs = now;
    }
    ui.loveUntilMs = 0;
    ui.temporaryMoodUntilMs = 0;
  }

  if (ui.idleMode && ui.temporaryMoodUntilMs == 0 && ui.loveUntilMs == 0 &&
      static_cast<long>(now - (ui.idleSinceMs + kCuriousAfterMs)) >= 0) {
    ui.mood = Mood::kCurious;
    ui.temporaryMoodUntilMs = now + kCuriousDurationMs;
    renderIdle();
    return;
  }

  if ((changed || temporaryExpired) && ui.loveUntilMs == 0 && ui.temporaryMoodUntilMs == 0) {
    const Mood nextMood = assessedMood();
    if (ui.mood != nextMood) {
      ui.mood = nextMood;
      if (ui.idleMode) renderIdle();
      else if (ui.page == Page::kHome) renderPage();
    }
  }

  if (!ui.idleMode) {
    if (ui.page == Page::kHome) {
      if (changed) updateHomeLabels();
    } else if ((changed && ui.page == Page::kCare) || ui.page == Page::kTrend) {
      renderPage();
    }
  }
}

void smartPlantReceiveHeart(const char* sender) {
  char event[34];
  snprintf(event, sizeof(event), "%s | sent a heart", sender && sender[0] ? sender : "Partner");
  addEvent(event);
  ui.mood = Mood::kLove;
  ui.loveUntilMs = millis() + 8000;
  if (ui.idleMode) {
    renderIdle();
  }
  if (ui.toast) {
    lv_obj_clear_flag(ui.toast, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui.toast);
    lv_timer_t* timer = lv_timer_create(hideToast, 4200, nullptr);
    lv_timer_set_repeat_count(timer, 1);
  }
  if (!ui.idleMode && (ui.page == Page::kCare || ui.page == Page::kHome)) renderPage();
}

void smartPlantStartDemo() {
  beginDemo();
}

bool smartPlantConsumeBacklightRequest() {
  if (!ui.requestBacklight) return false;
  ui.requestBacklight = false;
  return true;
}
