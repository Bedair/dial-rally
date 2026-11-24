// Motion Quest – M0: IMU Day 0 test
//
// Goal: prove the wrist IMU (Mini 6-axis IMU Unit, MPU6886) works as game
// input and collect data to tune the flick thresholds in GameConfig.h.
//
//  - Boot: I2C scan of PORT.A, then WHO_AM_I check. If the chip is not the
//    expected MPU6886 the screen shows what was read and the driver is not started.
//  - Screen flashes GREEN "JUMP" on flick up, ORANGE "DUCK" on flick down, with a beep.
//  - Counters: jumps, ducks, near misses (motion that opened a capture window
//    but did not fire). Press the Dial button to reset them.
//  - Serial (115200): one CSV line per capture window with signed peak values:
//      t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,action,fire_ms,n,gap_ms,up_dps,down_dps
//    (n = samples in the window, gap_ms = longest gap between them,
//     up/down = strongest rate in each direction on the flick axis, so a
//     wind-up or return swing is visible next to the main flick)
//    Status lines start with [MQ]; CSV lines have no prefix so they paste
//    straight into a spreadsheet.

#include <M5Dial.h>
#include <Wire.h>
#include "config/GameConfig.h"
#include "input/ImuSource.h"

static ImuSource imu;
static bool imuOk = false;
static String portScan;   // shown on screen if the IMU is missing

static M5Canvas canvas(&M5Dial.Display);   // full-frame buffer, pushed once per redraw

static uint32_t nJump = 0, nDuck = 0, nMiss = 0;
static uint32_t flashUntilMs = 0;
static Action flashAction = Action::None;
// A full-screen push takes ~12 ms and blocks IMU sampling, so routine redraws
// wait until no capture window is open. Only the flash itself is drawn at once.
static bool needRedraw = true;

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------
static String scanI2C() {
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
    Serial.printf("[MQ] I2C scan PORT.A: %s\n", found.c_str());
    return found;
}

static void initImu() {
    portScan = scanI2C();
    imuOk = imu.begin();
    Serial.printf("[MQ] IMU %s\n", imuOk ? "READY" : "NOT READY");
    if (imuOk) Serial.println("t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,action,fire_ms,n,gap_ms,up_dps,down_dps");
    needRedraw = true;
}

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------
static void drawNoImu() {
    canvas.setTextColor(TFT_RED);
    canvas.setTextSize(2);
    canvas.drawString(imu.present() ? "WRONG IMU" : "NO IMU", 120, 70);

    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(1);
    char line[48];
    snprintf(line, sizeof(line), "WHO_AM_I 0x%02X  (want 0x%02X)", imu.whoAmI(),
             cfg::IMU_WHO_AM_I_EXPECTED);
    canvas.drawString(line, 120, 105);
    snprintf(line, sizeof(line), "reg 0x00: 0x%02X  -> %s", imu.reg00(),
             Mpu6886::chipName(imu.whoAmI(), imu.reg00()));
    canvas.drawString(line, 120, 122);
    canvas.drawString(String("PORT.A: ") + portScan, 120, 145);
    canvas.drawString("press button to retry", 120, 175);
}

static void drawIdle() {
    canvas.setTextColor(TFT_GOLD);
    canvas.setTextSize(2);
    canvas.drawString("FLICK!", 120, 55);

    canvas.setTextColor(TFT_GREEN);
    canvas.drawString(String("Jump ") + nJump, 120, 95);
    canvas.setTextColor(TFT_ORANGE);
    canvas.drawString(String("Duck ") + nDuck, 120, 125);
    canvas.setTextColor(TFT_DARKGREY);
    canvas.drawString(String("Miss ") + nMiss, 120, 155);

    canvas.setTextSize(1);
    char line[40];
    snprintf(line, sizeof(line), "MPU6886 0x%02X  i2c err %lu", imu.whoAmI(),
             (unsigned long)imu.i2cErrors());
    canvas.drawString(line, 120, 190);
}

static void drawFlash(Action a) {
    canvas.fillScreen(a == Action::Jump ? TFT_GREEN : TFT_ORANGE);
    canvas.setTextColor(TFT_BLACK);
    canvas.setTextSize(4);
    canvas.drawString(actionName(a), 120, 120);
}

static void render() {
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextDatum(middle_center);
    if (!imuOk)            drawNoImu();
    else if (flashUntilMs) drawFlash(flashAction);
    else                   drawIdle();
    canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
static void logRecord(const FlickRecord& r) {
    Serial.printf("%lu,%.2f,%.2f,%.2f,%.0f,%.0f,%.0f,%s,%ld,%u,%lu,%.0f,%.0f\n", (unsigned long)r.tMs,
                  r.peakA[0], r.peakA[1], r.peakA[2], r.peakG[0], r.peakG[1], r.peakG[2],
                  actionName(r.action), (long)r.fireMs, (unsigned)r.samples,
                  (unsigned long)r.maxGapMs, r.upPeak, r.downPeak);
}

// ---------------------------------------------------------------------------
void setup() {
    auto c = M5.config();
    M5Dial.begin(c, true, false);   // encoder on, RFID off
    Serial.begin(115200);
    delay(800);                     // setup only: USB CDC + Grove unit power-up

    canvas.createSprite(240, 240);
    Wire.begin(cfg::PORTA_SDA, cfg::PORTA_SCL, cfg::I2C_FREQ_HZ);
    initImu();
    render();
}

void loop() {
    M5Dial.update();
    const uint32_t now = millis();

    if (M5Dial.BtnA.wasPressed()) {
        if (!imuOk) {
            initImu();              // retry without re-flashing
        } else {
            nJump = nDuck = nMiss = 0;
            Serial.println("[MQ] counters reset");
            needRedraw = true;
        }
    }

    if (imuOk) {
        ActionEvent ev;
        if (imu.poll(ev)) {
            if (ev.action == Action::Jump) nJump++;
            else                           nDuck++;
            flashAction = ev.action;
            flashUntilMs = now + cfg::FLASH_MS;
            M5Dial.Speaker.tone(ev.action == Action::Jump ? cfg::BEEP_JUMP_HZ : cfg::BEEP_DUCK_HZ,
                                cfg::BEEP_MS);
            render();               // immediately: this is the latency being measured
            needRedraw = false;
        }

        FlickRecord rec;
        if (imu.takeRecord(rec)) {
            logRecord(rec);
            if (rec.action == Action::None) {
                nMiss++;
                needRedraw = true;
            }
        }
    }

    if (flashUntilMs && (int32_t)(now - flashUntilMs) >= 0) {
        flashUntilMs = 0;
        needRedraw = true;
    }
    if (needRedraw && !imu.capturing()) {
        needRedraw = false;
        render();
    }
}
