// Motion Quest – Day 0 gesture test
//
// Goal: prove the PAJ7620 Gesture Unit works with the Dial and measure how
// usable it is for a reaction game, before any game code is written.
//
//  - Screen flashes GREEN "JUMP" on swipe up, ORANGE "DUCK" on swipe down.
//    Film hand + screen in slow motion: frames between hand start and flash = latency.
//  - Counts up / down / other gestures and repeats inside the cooldown window.
//  - Serial log (115200): one CSV line per gesture for later analysis.
//  - Press the Dial button to reset the counters.

#include <M5Dial.h>
#include <Wire.h>
#include "config/GameConfig.h"
#include "input/GestureSource.h"

static GestureSource gesture;
static bool sensorOk = false;

static uint32_t nJump = 0, nDuck = 0, nOther = 0, nRepeat = 0;
static uint32_t lastEventMs = 0;
static uint32_t flashUntilMs = 0;
static bool needRedraw = true;

static const char* rawName(Gesture g) {
    switch (g) {
        case GES_UP: return "UP";            case GES_DOWN: return "DOWN";
        case GES_LEFT: return "LEFT";        case GES_RIGHT: return "RIGHT";
        case GES_FORWARD: return "FORWARD";  case GES_BACKWARD: return "BACKWARD";
        case GES_CLOCKWISE: return "CW";     case GES_ANTICLOCKWISE: return "CCW";
        case GES_WAVE: return "WAVE";        default: return "NONE";
    }
}

static String scanResult;   // shown on screen if the sensor is missing

static String scanI2C(const char* label) {
    String found;
    for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            char buf[6];
            snprintf(buf, sizeof(buf), "%02X ", a);
            found += buf;
        }
    }
    if (found.isEmpty()) found = "none";
    Serial.printf("[MQ] I2C scan %s: %s\n", label, found.c_str());
    return found;
}

static void drawIdle() {
    auto& d = M5Dial.Display;
    d.fillScreen(TFT_BLACK);
    d.setTextDatum(middle_center);
    d.setTextSize(2);
    if (!sensorOk) {
        d.setTextColor(TFT_RED);
        d.drawString("NO SENSOR", 120, 105);
        d.setTextColor(TFT_WHITE);
        d.drawString("check PORT.A", 120, 135);
        d.setTextSize(1);
        d.drawString(scanResult, 120, 165);
        d.drawString("press to retry", 120, 185);
        return;
    }
    d.setTextColor(TFT_GOLD);
    d.drawString("READY", 120, 60);
    d.setTextColor(TFT_WHITE);
    d.drawString(String("Jump ") + nJump, 120, 100);
    d.drawString(String("Duck ") + nDuck, 120, 130);
    d.setTextColor(TFT_DARKGREY);
    d.drawString(String("Other ") + nOther + "  Rep " + nRepeat, 120, 165);
}

static void drawFlash(Action a) {
    auto& d = M5Dial.Display;
    d.fillScreen(a == Action::Jump ? TFT_GREEN : TFT_ORANGE);
    d.setTextDatum(middle_center);
    d.setTextColor(TFT_BLACK);
    d.setTextSize(4);
    d.drawString(actionName(a), 120, 120);
}

static void initSensor() {
    sensorOk = gesture.begin();
    Serial.printf("[MQ] gesture sensor %s (expect 0x%02X)\n",
                  sensorOk ? "OK" : "NOT FOUND", cfg::GESTURE_I2C_ADDR);
}

void setup() {
    auto c = M5.config();
    M5Dial.begin(c, true, false);
    Serial.begin(115200);
    delay(300);

    delay(500);   // give the Grove unit time to power up

    // Diagnostic sweep: both Grove ports, both pin orders. Shown on screen.
    struct Probe { const char* tag; int sda; int scl; };
    const Probe probes[] = {
        {"A",  13, 15}, {"Ax", 15, 13},    // PORT.A normal / swapped
        {"B",   2,  1}, {"Bx",  1,  2},    // PORT.B normal / swapped
    };
    String sweep;
    for (const auto& p : probes) {
        Wire.end();
        Wire.begin(p.sda, p.scl, cfg::I2C_FREQ_HZ);
        delay(20);
        String r = scanI2C(p.tag);
        sweep += String(p.tag) + ":" + r + " ";
    }
    Serial.printf("[MQ] sweep: %s\n", sweep.c_str());

    Wire.end();
    Wire.begin(cfg::PORTA_SDA, cfg::PORTA_SCL, cfg::I2C_FREQ_HZ);
    scanResult = sweep;
    initSensor();
    Serial.println("t_ms,raw,action,dt_since_last_ms,repeat");
    drawIdle();
}

void loop() {
    M5Dial.update();

    if (!sensorOk && M5Dial.BtnA.wasPressed()) {   // retry without re-flashing
        scanResult = String("A retry:") + scanI2C("PORT.A retry");
        initSensor();
        needRedraw = true;
        return;
    }

    if (M5Dial.BtnA.wasPressed()) {
        nJump = nDuck = nOther = nRepeat = 0;
        Serial.println("[MQ] counters reset");
        needRedraw = true;
    }

    ActionEvent ev;
    if (sensorOk && gesture.poll(ev)) {
        const uint32_t dt = lastEventMs ? ev.timestampMs - lastEventMs : 0;
        const bool repeat = lastEventMs && dt < cfg::GESTURE_COOLDOWN_MS;
        lastEventMs = ev.timestampMs;

        if (repeat) nRepeat++;
        if (ev.action == Action::Jump)      nJump++;
        else if (ev.action == Action::Duck) nDuck++;
        else                                nOther++;

        Serial.printf("%lu,%s,%s,%lu,%d\n", (unsigned long)ev.timestampMs,
                      rawName(gesture.lastRaw()), actionName(ev.action),
                      (unsigned long)dt, repeat ? 1 : 0);

        if (ev.action == Action::Jump || ev.action == Action::Duck) {
            drawFlash(ev.action);
            M5Dial.Speaker.tone(ev.action == Action::Jump ? 2400 : 1200, 40);
            flashUntilMs = millis() + 300;
        } else {
            needRedraw = true;
        }
    }

    if (flashUntilMs && millis() > flashUntilMs) {
        flashUntilMs = 0;
        needRedraw = true;
    }
    if (needRedraw && !flashUntilMs) {
        needRedraw = false;
        drawIdle();
    }
}
