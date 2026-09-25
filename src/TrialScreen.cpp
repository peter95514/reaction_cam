#include "TrialScreen.h"

#include <imgui.h>

#include <random>

#include "Screen.h"
#include "Ui.h"

void TrialScreen::enter() {
    state_ = State::Idle;
    this->idleEnd_ = Clock::now() + toDur(cfg_.countdown);
}

void TrialScreen::exit() {}

Clock::duration TrialScreen::toDur(float sec) {
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<float>(sec));
}

void TrialScreen::drawLight() {
    const ImU32 kOff = IM_COL32(45, 50, 60, 255);
    const ImU32 kOn = IM_COL32(60, 220, 100, 255);
    const ImU32 kRim = IM_COL32(255, 255, 255, 50);

    const bool running = state_ != State::Idle;

    bool light[2] = {0, 0};

    if (running && islig_) light[side_ == Side::Left ? 0 : 1] = true;

    const ImVec2 wp = ImGui::GetWindowPos();
    const ImVec2 ws = ImGui::GetWindowSize();
    const float cx = wp.x + ws.x * 0.5f;
    const float cy = wp.y + ws.y * 0.48f;
    const float gap = 190.f;  // 圓心到視窗中線的距離
    const float r = 90.f;
    const char* labels[2] = {"左", "右"};

    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int i = 0; i < 2; ++i) {
        ImVec2 c{cx + (i == 0 ? -gap : gap), cy};
        if (light[i]) {
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
    } else {
        islig_ = true;
        nextChange_ = now + toDur(cfg_.onSec);
        side_ = std::uniform_int_distribution<int>(0, 1)(rng_) == 0
                    ? Side::Left
                    : Side::Right;
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
    }

    ui::beginFullscreen("##Trial");
    drawLight();
    ImGui::End();

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) next = ScreenId::Menu;
    return next;
}
