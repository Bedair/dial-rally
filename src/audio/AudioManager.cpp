#include "audio/AudioManager.h"
#include "config/GameConfig.h"
#include <M5Dial.h>

namespace {

using Note = AudioManager::Note;

// Original jingles, built from plain square-ish buzzer tones.
constexpr Note TICK[] = {{1800, 12}};
constexpr Note CONFIRM[] = {{1200, 30}, {1800, 45}};
constexpr Note COUNT[] = {{880, 90}};
constexpr Note GO[] = {{1760, 240}};
constexpr Note JUMP[] = {{700, 25}, {1000, 25}, {1400, 35}};                 // rising chirp
constexpr Note COIN[] = {{1976, 45}, {2637, 100}};                           // bright "bling"
constexpr Note MILESTONE[] = {{1047, 70}, {1319, 70}, {1568, 70}, {2093, 170}};
constexpr Note CRASH[] = {{330, 60}, {220, 80}, {150, 120}, {90, 220}};      // falling crunch
constexpr Note HIGHSCORE[] = {{1568, 100}, {0, 40}, {1568, 80}, {2093, 280}};

struct Effect { const Note* notes; uint8_t len; uint8_t prio; };
#define FX(notes, prio) {notes, sizeof(notes) / sizeof(notes[0]), prio}

// Indexed by Sfx.
constexpr Effect EFFECTS[] = {
    FX(TICK, 1),  FX(CONFIRM, 2), FX(COUNT, 2),     FX(GO, 3),        FX(JUMP, 2),
    FX(COIN, 3),  FX(MILESTONE, 4), FX(CRASH, 5),   FX(HIGHSCORE, 5),
};

}  // namespace

void AudioManager::begin() {
    M5Dial.Speaker.setVolume(cfg::SPEAKER_VOLUME);
}

void AudioManager::play(Sfx s, uint32_t nowMs) {
    const Effect& e = EFFECTS[(int)s];
    if (seq_ && e.prio < prio_) return;          // a more important sound is playing
    seq_ = e.notes;
    len_ = e.len;
    idx_ = 0;
    prio_ = e.prio;
    nextMs_ = nowMs;
    update(nowMs);                               // first note right away
}

void AudioManager::update(uint32_t nowMs) {
    if (!seq_ || (int32_t)(nowMs - nextMs_) < 0) return;
    if (idx_ >= len_) {                          // finished
        seq_ = nullptr;
        prio_ = 0;
        return;
    }
    const Note& n = seq_[idx_++];
    if (n.hz) M5Dial.Speaker.tone(n.hz, n.ms);
    nextMs_ = nowMs + n.ms;
}
