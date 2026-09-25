#include "TrialController.h"

#include <memory>
#include <opencv2/core/types.hpp>
#include <stdexcept>
#include <thread>

#include "ActionEvaluator.h"
#include "MotionDetector.h"

TrialController::TrialController()
    : leftEval_(0.4f, 80.f, 0.5f), rightEval_(0.4f, 80.f, 0.5f) {
    leftEval_.changerule({
        {"L_shoulder", L_HIP, L_SHOULDER, L_ELBOW, 140.f, 180.f, 2.0f},
        {"L_elbow", L_SHOULDER, L_ELBOW, L_WRIST, 130.f, 180.f, 1.0f},
    });
    rightEval_.changerule({
        {"R_shoulder", R_HIP, R_SHOULDER, R_ELBOW, 140.f, 180.f, 2.0f},
        {"R_elbow", R_SHOULDER, R_ELBOW, R_WRIST, 130.f, 180.f, 1.0f},
    });
}

TrialController::~TrialController() {
    stop();
}

void TrialController::start() {
    if (running_) return;
    if (!cap_.open(0)) throw std::runtime_error("cant open cam");

    detector_ = std::make_unique<MotionDetector>(500, 25.0, 0, 1000.0);
    running_ = true;
    worker_ = std::thread(&TrialController::workerLoop, this);
}

void TrialController::stop() {
    if (!running_) return;
    running_ = false;
    if (worker_.joinable()) worker_.join();
    cap_.release();
    detector_.reset();
    std::lock_guard<std::mutex> lk(mtx_);
}
bool TrialController::isPassed(Side side) const {
    std::lock_guard<std::mutex> lk(mtx_);
    return side == Side::Left ? lpass_ : rpass_;
}

void TrialController::workerLoop() {
    cv::Mat frame;
    bool lp = false;
    bool rp = false;
    while (running_) {
        if (!cap_.read(frame) || frame.empty()) break;

        std::vector<cv::Rect> rois = detector_->get_rois(frame);
        if (!rois.empty()) {
            cv::Rect roi = rois[0] & cv::Rect(0, 0, frame.cols, frame.rows);
            if (roi.width > 0 && roi.height > 0) {
                detector_->detect_skeleton(frame(roi));
                const auto kp = detector_->get_keypoints();
                EvalResult l = leftEval_.evaluate(kp);
                EvalResult r = rightEval_.evaluate(kp);
                lp = l.valid && l.passed;
                rp = r.valid && r.passed;
            }
        }
        std::lock_guard<std::mutex> lk(mtx_);
        lpass_ = lp;
        rpass_ = rp;
    }
}
