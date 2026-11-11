#include "TrialController.h"

#include <memory>
#include <mutex>
#include <opencv2/core/types.hpp>
#include <stdexcept>
#include <thread>

#include "ActionEvaluator.h"
#include "MotionDetector.h"

TrialController::TrialController()
    : evaluator_(0.4f, 80.f, 0.5f) {
}

TrialController::~TrialController() {
    stop();
}
void TrialController::resetRecords() {
    std::lock_guard<std::mutex> lk(mtx_);
    records_.clear();
}

std::vector<FrameResult> TrialController::snapshotRecords() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return records_;
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
bool TrialController::isPassed() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return passed_;
}

void TrialController::setAction(const Action& action){
    std::lock_guard<std::mutex> lk(mtx_);
    evaluator_.changerule(action);
    hasAction_ = !action.empty();
    passed_ = false;
    records_.clear();
}

void TrialController::workerLoop() {
    cv::Mat frame;
    while (running_) {
        if (!cap_.read(frame) || frame.empty()) break;
        const auto captured = Clock::now();

        std::vector<Keypoint> kp;
        std::vector<cv::Rect> rois = detector_->get_rois(frame);
        if (!rois.empty()) {
            cv::Rect roi = rois[0] & cv::Rect(0, 0, frame.cols, frame.rows);
            if (roi.width > 0 && roi.height > 0) {
                detector_->detect_skeleton(frame(roi));
                kp = detector_->get_keypoints();
            }
        }
        std::lock_guard<std::mutex> lk(mtx_);
        EvalResult r = evaluator_.evaluate(kp);
        passed_ = r.valid && r.passed;
        if (hasAction_) records_.push_back({captured, passed_});
    }
}
