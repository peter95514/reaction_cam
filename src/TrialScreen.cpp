#include "TrialScreen.h"

#include <imgui.h>

#include <cstdio>
#include <random>

#include "Screen.h"
#include "Ui.h"


static std::vector<Cue> defaultCues() {
    Action left = {
        {"L_shoulder", L_HIP, L_SHOULDER, L_ELBOW, 140.f, 180.f, 2.0f},
        {"L_elbow", L_SHOULDER, L_ELBOW, L_WRIST, 130.f, 180.f, 1.0f},
    };
    Action right = {
        {"R_shoulder", R_HIP, R_SHOULDER, R_ELBOW, 140.f, 180.f, 2.0f},
        {"R_elbow", R_SHOULDER, R_ELBOW, R_WRIST, 130.f, 180.f, 1.0f},
    };
    return {
        Cue{LightSignal{0, LightColor::Green}, left},
        Cue{LightSignal{1, LightColor::Green}, right},
    };
}

void TrialScreen::enter() {
    state_ = State::Idle;
    islig_ = false;
    this->idleEnd_ = Clock::now() + toDur(cfg_.countdown);
    coordinator_.start();
}

void TrialScreen::exit() {
    coordinator_.stop();
}

Clock::duration TrialScreen::toDur(float sec) {
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<float>(sec));
}

TrialScreen::TrialScreen() {
    cfg_.cues = defaultCues();
}

static ImU32 toImColor(LightColor c) {
    switch (c) {
        case LightColor::Green:
            return IM_COL32(60, 220, 100, 255);
        case LightColor::Red:
            return IM_COL32(230, 70, 70, 255);
        case LightColor::Blue:
            return IM_COL32(70, 140, 255, 255);
        case LightColor::Yellow:
            return IM_COL32(240, 200, 60, 255);
    }
    return IM_COL32(255, 255, 255, 255);
}

void TrialScreen::drawLight() {
    const ImU32 kOff = IM_COL32(45, 50, 60, 255);
    const ImU32 kOn = toImColor(cue_.first.color);
    const ImU32 kRim = IM_COL32(255, 255, 255, 50);

    const bool running = state_ != State::Idle;
    const int n = cfg_.lightCount;

    const ImVec2 wp = ImGui::GetWindowPos();
    const ImVec2 ws = ImGui::GetWindowSize();
    const float cx = wp.x + ws.x * 0.5f;
    const float cy = wp.y + ws.y * 0.48f;
    const float r = 90.f;
    // 燈多時縮小間距，避免超出視窗；n=2 時與原本一樣 (±190)
    const float spacing =
        n > 1 ? std::min(380.f, (ws.x - 2.f * (r + 40.f)) / (n - 1)) : 0.f;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int i = 0; i < n; ++i) {
        ImVec2 c{cx + (i - (n - 1) * 0.5f) * spacing, cy};
        const bool lit = running && islig_ && i == cue_.first.index;
        if (lit) {
            dl->AddCircleFilled(c, r + 24.f, (kOn & 0x00FFFFFF) | (40u << 24),
                                64);
            dl->AddCircleFilled(c, r, kOn, 64);
        } else {
            dl->AddCircleFilled(c, r, kOff, 64);
        }
        dl->AddCircle(c, r, kRim, 64, 3.f);
    }
}

Clock::duration TrialScreen::randomGap() {
    std::uniform_real_distribution<float> d(cfg_.gapMin, cfg_.gapMax);
    return toDur(d(rng_));
}


void TrialScreen::updateLight(Clock::time_point now) {
    if (now < this->nextChange_) return;

    if (islig_) {
        islig_ = false;
        nextChange_ = now + randomGap();
        coordinator_.notifyLightOff();
    } else {
        if (cfg_.cues.empty()) return;
        std::uniform_int_distribution<size_t> pick(0, cfg_.cues.size() - 1);
        cue_ = cfg_.cues[pick(rng_)];
        islig_ = true;
        nextChange_ = now + toDur(cfg_.onSec);

        coordinator_.notifyLightOn(cue_, now);
    }
}

void TrialScreen::beginRun(Clock::time_point now) {
    state_ = State::On;
    runEnd_ = now + toDur(cfg_.duration);
    nextChange_ = now + randomGap();
}

ScreenId TrialScreen::tick() {
    ScreenId next = ScreenId::Trial;
    const auto now = Clock::now();
    if (this->state_ == State::Idle && now >= idleEnd_) beginRun(now);

    if (this->state_ != State::Idle) {
        if (now >= this->runEnd_) {
            return ScreenId::Menu;
        }
        updateLight(now);
        if (auto result = coordinator_.poll()) {
            float ms = std::chrono::duration<float, std::milli>(result->latency)
                           .count();

            std::printf("燈 %d 反應時間: %.1f ms (passed=%d)\n",
            cue_.first.index, ms, result->passed);
        }
    }

    ui::beginFullscreen("##Trial");
    drawLight();
    ImGui::End();

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) next = ScreenId::Menu;
    return next;
}
