// Dial Rally – an arcade racing game in a single knob (M5Stack Dial).
//
// main.cpp only wires the modules together and runs the frame loop:
//   KnobSource (input) -> GameController (state machine) -> DisplayManager (canvas)
// Each frame: read input, update the game, draw into the canvas, push once.
// Frames are paced with millis(); there is no delay() in the loop.

#include <M5Dial.h>
#include "config/GameConfig.h"
#include "input/KnobSource.h"
#include "display/DisplayManager.h"
#include "display/FrameStats.h"
#include "game/GameController.h"

static KnobSource knob;
static DisplayManager display;
static GameController game(knob, display);
static FrameStats stats;

static uint32_t lastFrameMs = 0;

void setup() {
    auto c = M5.config();
    M5Dial.begin(c, true, false);   // encoder on, RFID off
    Serial.begin(115200);
    delay(500);                     // setup only: give USB CDC time to attach

    display.begin();
    knob.begin();
    lastFrameMs = millis();
    game.begin(lastFrameMs);
    stats.begin(lastFrameMs);
    Serial.printf("[MQ] Dial Rally ready, %.1f px/step\n", cfg::KNOB_PX_PER_STEP);
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
}
