// Motion Quest – The Lost Temple
// Bring-up check: display, encoder and button on the M5Stack Dial.

#include <M5Dial.h>

static long lastEncoder = 0;

void drawStatus(long enc, bool pressed) {
    auto& d = M5Dial.Display;
    d.fillScreen(TFT_BLACK);
    d.setTextDatum(middle_center);
    d.setTextColor(TFT_GOLD);
    d.setTextSize(2);
    d.drawString("MOTION QUEST", d.width() / 2, 80);
    d.setTextColor(TFT_WHITE);
    d.drawString(String("Encoder: ") + enc, d.width() / 2, 120);
    d.drawString(pressed ? "Button: DOWN" : "Button: up", d.width() / 2, 155);
}

void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);   // encoder on, RFID off
    Serial.begin(115200);
    Serial.println("[MQ] boot ok");
    drawStatus(0, false);
}

void loop() {
    M5Dial.update();

    long enc = M5Dial.Encoder.read();
    bool pressed = M5Dial.BtnA.isPressed();
    static bool lastPressed = false;

    if (enc != lastEncoder || pressed != lastPressed) {
        lastEncoder = enc;
        lastPressed = pressed;
        drawStatus(enc, pressed);
        if (pressed) M5Dial.Speaker.tone(2000, 40);
        Serial.printf("[MQ] t=%lu enc=%ld btn=%d\n", millis(), enc, pressed);
    }
}
