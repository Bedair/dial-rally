// The Lost Temple – M0: knob input test screen
//
// Goal: prove the Dial's own knob, button and touchscreen work as game input
// through KnobSource, and measure the encoder resolution.
//
//  - Rim track with an explorer dot. Turning the knob moves the target angle by
//    KNOB_DEG_PER_STEP per step; the drawn dot eases toward it (EASE_TAU_MS).
//  - Press BtnA (or tap the screen): rim flashes and the buzzer beeps.
//  - Encoder measurement: hold BtnA ~1 s to set a mark, turn the knob exactly
//    one full turn, hold BtnA again. Serial prints the counts per full turn.
//    The altar in the centre shows the counts since the mark.
//  - Serial (115200), all lines prefixed [MQ]: turns, presses, measurement
//    results, free heap at boot and the average frame time every 5 s.
//
// Angle convention (used by the game later too): 0° = 12 o'clock, clockwise +.

#include <M5Dial.h>
#include <math.h>
#include "config/GameConfig.h"
#include "input/KnobSource.h"

static KnobSource knob;
static M5Canvas canvas(&M5Dial.Display);   // full-frame buffer, pushed once per frame

static float targetAngle = 0.0f;   // where the knob says the explorer is, 0..360
static float drawAngle = 0.0f;     // eased position actually drawn, 0..360

static uint32_t flashUntilMs = 0;
static Origin flashOrigin = Origin::Button;
static uint32_t nPress = 0, nTouch = 0;

static bool measuring = false;     // true between the two long presses
static int32_t markCount = 0;      // KnobSource::rawCount() at the mark

static uint32_t lastFrameMs = 0;
static uint32_t statFrames = 0, statWorkMs = 0, statStartMs = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static float wrap360(float a) {
    a = fmodf(a, 360.0f);
    return a < 0 ? a + 360.0f : a;
}

// Signed shortest difference b - a, in -180..180.
static float angleDiff(float a, float b) {
    float d = wrap360(b - a);
    return d > 180.0f ? d - 360.0f : d;
}

// Point on a circle around the screen centre for an angle in degrees.
static void polar(float deg, float r, int& x, int& y) {
    const float rad = deg * (float)DEG_TO_RAD;
    x = cfg::CENTER_X + (int)lroundf(r * sinf(rad));
    y = cfg::CENTER_Y - (int)lroundf(r * cosf(rad));
}

// ---------------------------------------------------------------------------
// Setup helpers
// ---------------------------------------------------------------------------
static void createCanvas() {
    Serial.printf("[MQ] free heap before canvas: %u B\n", (unsigned)ESP.getFreeHeap());
    canvas.setColorDepth(16);
    if (!canvas.createSprite(cfg::SCREEN_W, cfg::SCREEN_H)) {
        Serial.println("[MQ] 16-bit canvas failed, falling back to 8-bit");
        canvas.setColorDepth(8);
        canvas.createSprite(cfg::SCREEN_W, cfg::SCREEN_H);
    }
    Serial.printf("[MQ] canvas %dx%d @ %d bit, free heap: %u B\n", (int)canvas.width(),
                  (int)canvas.height(), (int)canvas.getColorDepth(), (unsigned)ESP.getFreeHeap());
}

// ---------------------------------------------------------------------------
// Input handling
// ---------------------------------------------------------------------------
static void onLongPress() {
    const int32_t raw = knob.rawCount();
    if (!measuring) {
        measuring = true;
        markCount = raw;
        Serial.println("[MQ] measure: mark set - turn exactly one full turn, then hold again");
    } else {
        measuring = false;
        const int32_t counts = raw - markCount;
        Serial.printf("[MQ] measure: %ld counts per full turn (%ld steps at %ld counts/step)\n",
                      (long)counts, (long)(counts / cfg::ENCODER_COUNTS_PER_STEP),
                      (long)cfg::ENCODER_COUNTS_PER_STEP);
    }
    M5Dial.Speaker.tone(cfg::BEEP_HOLD_HZ, cfg::BEEP_MS * 3);
}

static void handleInput(uint32_t now) {
    knob.update(now);
    ActionEvent ev;
    while (knob.poll(ev)) {
        switch (ev.action) {
            case Action::Turn:
                targetAngle = wrap360(targetAngle + ev.angleDeg);
                Serial.printf("[MQ] turn %+ld steps, raw %ld, target %.1f deg\n", (long)ev.steps,
                              (long)knob.rawCount(), targetAngle);
                break;
            case Action::Press:
                if (ev.origin == Origin::Touch) nTouch++; else nPress++;
                flashOrigin = ev.origin;
                flashUntilMs = now + cfg::FLASH_MS;
                M5Dial.Speaker.tone(ev.origin == Origin::Touch ? cfg::BEEP_TOUCH_HZ
                                                               : cfg::BEEP_PRESS_HZ,
                                    cfg::BEEP_MS);
                Serial.printf("[MQ] press (%s) at %lu ms\n", originName(ev.origin),
                              (unsigned long)ev.timestampMs);
                break;
            case Action::LongPress:
                onLongPress();
                break;
            default:
                break;
        }
    }
}

// Move the drawn angle toward the target. 1 - e^(-dt/tau) makes the easing
// independent of the frame rate.
static void easeExplorer(uint32_t dtMs) {
    const float k = 1.0f - expf(-(float)dtMs / cfg::EASE_TAU_MS);
    const float d = angleDiff(drawAngle, targetAngle);
    drawAngle = fabsf(d) < 0.05f ? targetAngle : wrap360(drawAngle + d * k);
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
static void render(uint32_t now) {
    const bool flashing = flashUntilMs && (int32_t)(now - flashUntilMs) < 0;
    const uint16_t flashColor = flashOrigin == Origin::Touch ? TFT_CYAN : TFT_GOLD;

    canvas.fillScreen(flashing ? canvas.color565(40, 30, 0) : TFT_BLACK);

    // Rim track with a tick every 30° (12 o'clock tick brighter).
    const int rOut = cfg::RIM_RADIUS + cfg::RIM_WIDTH / 2;
    const int rIn = cfg::RIM_RADIUS - cfg::RIM_WIDTH / 2;
    canvas.fillArc(cfg::CENTER_X, cfg::CENTER_Y, rOut, rIn, 0, 360,
                   flashing ? flashColor : canvas.color565(70, 60, 45));
    for (int deg = 0; deg < 360; deg += 30) {
        int x0, y0, x1, y1;
        polar(deg, rIn, x0, y0);
        polar(deg, rOut, x1, y1);
        canvas.drawLine(x0, y0, x1, y1, deg == 0 ? TFT_WHITE : canvas.color565(110, 95, 70));
    }

    // Altar: counts since the mark while measuring, else raw count.
    canvas.fillCircle(cfg::CENTER_X, cfg::CENTER_Y, cfg::ALTAR_RADIUS, canvas.color565(45, 40, 32));
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(measuring ? TFT_GREEN : TFT_WHITE);
    canvas.setTextSize(2);
    const int32_t shown = measuring ? knob.rawCount() - markCount : knob.rawCount();
    canvas.drawNumber(shown, cfg::CENTER_X, cfg::CENTER_Y - 6);
    canvas.setTextSize(1);
    canvas.setTextColor(TFT_LIGHTGREY);
    canvas.drawString(measuring ? "since mark" : "raw count", cfg::CENTER_X, cfg::CENTER_Y + 14);

    // Labels between altar and rim.
    char line[32];
    snprintf(line, sizeof(line), "btn %lu  touch %lu", (unsigned long)nPress, (unsigned long)nTouch);
    canvas.drawString(line, cfg::CENTER_X, cfg::CENTER_Y + 56);
    canvas.drawString(measuring ? "turn 1x, hold to stop" : "hold = measure", cfg::CENTER_X,
                      cfg::CENTER_Y - 56);

    // Target marker (thin) and eased explorer dot.
    int tx, ty, ex, ey;
    polar(targetAngle, rIn - 6, tx, ty);
    canvas.fillCircle(tx, ty, 2, TFT_DARKGREY);
    polar(drawAngle, cfg::RIM_RADIUS, ex, ey);
    canvas.fillCircle(ex, ey, flashing ? cfg::EXPLORER_RADIUS + 3 : cfg::EXPLORER_RADIUS,
                      flashing ? TFT_WHITE : TFT_ORANGE);

    canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
void setup() {
    auto c = M5.config();
    M5Dial.begin(c, true, false);   // encoder on, RFID off
    Serial.begin(115200);
    delay(500);                     // setup only: give USB CDC time to attach

    M5Dial.Speaker.setVolume(cfg::SPEAKER_VOLUME);
    createCanvas();
    knob.begin();
    Serial.printf("[MQ] M0 knob test ready, %.3f deg/step, ease tau %.0f ms\n",
                  cfg::KNOB_DEG_PER_STEP, cfg::EASE_TAU_MS);

    lastFrameMs = statStartMs = millis();
}

void loop() {
    const uint32_t now = millis();
    if (now - lastFrameMs < cfg::FRAME_MS) return;   // frame cap, no delay()
    const uint32_t dt = now - lastFrameMs;
    lastFrameMs = now;

    M5Dial.update();
    handleInput(now);
    easeExplorer(dt);
    render(now);
    if (flashUntilMs && (int32_t)(now - flashUntilMs) >= 0) flashUntilMs = 0;

    // Frame-time stats: work time per frame (input + render + push).
    statWorkMs += millis() - now;
    statFrames++;
    if (now - statStartMs >= 5000) {
        Serial.printf("[MQ] frames %lu in %lu ms, avg work %.1f ms/frame\n",
                      (unsigned long)statFrames, (unsigned long)(now - statStartMs),
                      (float)statWorkMs / statFrames);
        statFrames = statWorkMs = 0;
        statStartMs = now;
    }
}
