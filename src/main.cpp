// Dial Rally – an arcade racing game in a single knob (M5Stack Dial).
//
// main.cpp only wires the modules together and runs the frame loop:
//   KnobSource (input) -> GameController (state machine) -> DisplayManager (canvas)
// Each frame: read input, update the game, draw into the canvas, push once.
// Frames are paced with millis(); there is no delay() in the loop.
//
// Soak build (MQ_SOAK=1): AutoPilotSource replaces the knob so the game plays
// itself for long-run stability tests (docs/TESTING.md).

#include <M5Dial.h>
#include <esp_system.h>
#include "config/GameConfig.h"
#include "input/KnobSource.h"
#include "input/AutoPilotSource.h"
#include "display/DisplayManager.h"
#include "display/FrameStats.h"
#include "game/GameController.h"
#include "storage/ScoreManager.h"
#include "audio/AudioManager.h"

static KnobSource knob;
static AutoPilotSource autopilot;
static IInputSource& input = cfg::SOAK_TEST ? static_cast<IInputSource&>(autopilot)
                                            : static_cast<IInputSource&>(knob);
static DisplayManager display;
static ScoreManager scores;
static AudioManager audio;
static GameController game(input, display, scores, audio);
static FrameStats stats;

static uint32_t lastFrameMs = 0;

// Why the chip last started: a soak test must only ever show a power-on or
// USB reset, never a watchdog, panic or brown-out.
static const char* resetReason() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "power-on";
        case ESP_RST_SW:        return "software";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INTERRUPT WATCHDOG";
        case ESP_RST_TASK_WDT:  return "TASK WATCHDOG";
        case ESP_RST_WDT:       return "WATCHDOG";
        case ESP_RST_BROWNOUT:  return "BROWN-OUT";
        case ESP_RST_USB:       return "usb";
        default:                return "other";
    }
}

void setup() {
    auto c = M5.config();
    M5Dial.begin(c, true, false);   // encoder on, RFID off
    Serial.begin(115200);
    delay(500);                     // setup only: give USB CDC time to attach
    Serial.printf("[MQ] boot, reset reason: %s\n", resetReason());

    display.begin();
    autopilot.attach(&game);
    input.begin();
    scores.begin();
    audio.begin();
    lastFrameMs = millis();
    game.begin(lastFrameMs);
    stats.begin(lastFrameMs);
    Serial.printf("[MQ] Dial Rally ready (%s input), %.1f px/step\n", input.name(),
                  cfg::KNOB_PX_PER_STEP);
}

void loop() {
    const uint32_t now = millis();
    if (now - lastFrameMs < cfg::FRAME_MS) return;   // frame cap
    const uint32_t dt = now - lastFrameMs;
    lastFrameMs = now;

    M5Dial.update();          // buttons + touch; KnobSource reads the result
    game.update(now, dt);
    game.render(now);
    display.push();

    stats.add(now, millis() - now);
    stats.maybeReport(now, game.runs());
}
