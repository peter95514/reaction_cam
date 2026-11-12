#include "TrialScreen.h"

#include <imgui.h>

#include <cstdio>
#include <new>
#include <random>
#include <numeric>
#include <algorithm>

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

static Action defaultRelaxed() {
    return {
        {"L_down", L_HIP, L_SHOULDER, L_ELBOW, 0.f, 50.f, 1.0f},
        {"R_down", R_HIP, R_SHOULDER, R_ELBOW, 0.f, 50.f, 1.0f},
    };
}

void TrialScreen::enter() {
    state_ = State::Idle;
    done_ = 0;
    islig_ = false;
    this->idleEnd_ = Clock::now() + toDur(cfg_.countdown);
    coordinator_.setRelaxedPose(cfg_.relaxed);
    latenciesMs_.clear();
    coordinator_.start();
}

void TrialScreen::exit() {
    coordinator_.stop();
    cv::destroyWindow("DEBUG camera");  // DEBUG
}

Clock::duration TrialScreen::toDur(float sec) {
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<float>(sec));
}

TrialScreen::TrialScreen() {
    cfg_.cues = defaultCues();
    cfg_.relaxed = defaultRelaxed();
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

void TrialScreen::drawReaction() {
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.78f);
    if (latenciesMs_.empty()) {
        ui::centeredText("等待燈亮…");
        return;
    }

    char buf[96];
    std::snprintf(buf, sizeof(buf), "%.0f ms", latenciesMs_.back());
    ImGui::SetWindowFontScale(2.2f);
    ui::centeredText(buf);
    ImGui::SetWindowFontScale(1.0f);

    const float avg =
        std::accumulate(latenciesMs_.begin(), latenciesMs_.end(), 0.f) /
        latenciesMs_.size();
    std::snprintf(buf, sizeof(buf), "第 %d / %d 次    平均 %.0f ms", done_,
                  cfg_.targetCount, avg);
    ui::centeredText(buf);
}

bool TrialScreen::drawResult() {
    ImGui::Dummy({0, 40});
    ImGui::SetWindowFontScale(2.0f);
    ui::centeredText("測試完成");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy({0, 20});

    char buf[96];
    if (!latenciesMs_.empty()) {
        const float sum =
            std::accumulate(latenciesMs_.begin(), latenciesMs_.end(), 0.f);
        const float best =
            *std::min_element(latenciesMs_.begin(), latenciesMs_.end());
        std::snprintf(buf, sizeof(buf), "平均 %.0f ms    最佳 %.0f ms",
                      sum / latenciesMs_.size(), best);
        ui::centeredText(buf);
        ImGui::Dummy({0, 10});
    }
    for (size_t i = 0; i < latenciesMs_.size(); ++i) {
        std::snprintf(buf, sizeof(buf), "第 %zu 次：%.0f ms", i + 1,
                      latenciesMs_[i]);
        ui::centeredText(buf);
    }

    ImGui::Dummy({0, 20});
    return ui::centeredButton("回選單", 340, 68);
}

Clock::duration TrialScreen::randomGap() {
    std::uniform_real_distribution<float> d(cfg_.gapMin, cfg_.gapMax);
    return toDur(d(rng_));
}


void TrialScreen::updateLight(Clock::time_point now) {
    // 亮燈中：不設逾時，只等 tick() 裡 poll() 回報完成後由 turnOff() 熄燈
    if (islig_) return;

    if (now < this->nextChange_) return;
    if (cfg_.cues.empty()) return;
    // 隨機間隔已到，但還沒回到放鬆姿勢就繼續等
    if (!coordinator_.isRelaxed(cfg_.relaxFrames)) return;

    std::uniform_int_distribution<size_t> pick(0, cfg_.cues.size() - 1);
    cue_ = cfg_.cues[pick(rng_)];
    islig_ = true;
    coordinator_.notifyLightOn(cue_, now);
}

void TrialScreen::beginRun(Clock::time_point now) {
    state_ = State::On;
    done_ = 0;
    nextChange_ = now + randomGap();
}

ScreenId TrialScreen::tick() {
    ScreenId next = ScreenId::Trial;
    const auto now = Clock::now();
    if (this->state_ == State::Idle && now >= idleEnd_) beginRun(now);

        if (this->state_ == State::On) {
        updateLight(now);
        if (auto result = coordinator_.poll()) {
            float ms = std::chrono::duration<float, std::milli>(result->latency)
                           .count();
            latenciesMs_.push_back(ms);
            std::printf("燈 %d 反應時間: %.1f ms (passed=%d)\n",
            cue_.first.index, ms, result->passed);

            ++done_;
            islig_ = false;                      // 熄燈
            coordinator_.notifyLightOff();
            nextChange_ = now + randomGap();     // 排下一次亮燈
        }
        if (done_ >= cfg_.targetCount) {
            state_ = State::Done;
            islig_ = false;
            coordinator_.notifyLightOff();
        }
    }

    ui::beginFullscreen("##Trial");
    if (state_ == State::Done) {
        if (drawResult()) next = ScreenId::Menu;
    } else {
        drawLight();
        drawReaction();
    }
    ImGui::End();

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) next = ScreenId::Menu;
        cv::Mat dbg;
    if (coordinator_.debugFrame(dbg)) cv::imshow("DEBUG camera", dbg);
    cv::waitKey(1);
    return next;
}
