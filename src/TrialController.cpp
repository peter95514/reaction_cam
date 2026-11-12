#include "TrialController.h"

#include <memory>
#include <mutex>
#include <opencv2/core/types.hpp>
#include <stdexcept>
#include <thread>

#include "ActionEvaluator.h"
#include "MotionDetector.h"

TrialController::TrialController()
    : evaluator_(0.4f, 80.f, 0.5f), relaxEvaluator_(0.4f, 80.0f, 0.5f) {
}

// DEBUG
bool TrialController::debugFrame(cv::Mat& out) const {
    std::lock_guard<std::mutex> lk(mtx_);
    if (debugFrame_.empty()) return false;
    debugFrame_.copyTo(out);
    return true;
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

void TrialController::setRelaxPose(const Action& pose) {
    std::lock_guard<std::mutex> lk(mtx_);
    relaxEvaluator_.changerule(pose);
    relaxFrames_ = 0;
}

bool TrialController::isRelaxed(int minFrames){
    std::lock_guard<std::mutex> lk(mtx_);
    return relaxFrames_ > minFrames;
}

void TrialController::workerLoop() {
    cv::Mat frame;
    int dbgFrame = 0;

    static const int kPairs[][2] = {{5, 6},  {5, 7},   {7, 9},   {6, 8},
                                    {8, 10}, {5, 11},  {6, 12},  {11, 12},
                                    {11, 13}, {13, 15}, {12, 14}, {14, 16}};

    while (running_) {
        if (!cap_.read(frame) || frame.empty()) break;
        const auto captured = Clock::now();

        cv::Rect roi;

        std::vector<Keypoint> kp;
        std::vector<cv::Rect> rois = detector_->get_rois(frame);
        if (!rois.empty()) {
            roi = rois[0] & cv::Rect(0, 0, frame.cols, frame.rows);
            if (roi.width > 0 && roi.height > 0) {
                detector_->detect_skeleton(frame(roi));
                kp = detector_->get_keypoints();
            }
        }


        // DEBUG: 在影格副本上畫 ROI、骨架、每個關鍵點的信心值
        cv::Mat dbg = frame.clone();
        if (roi.area() > 0) {
            cv::rectangle(dbg, roi, {0, 255, 0}, 2);
            const cv::Point off = roi.tl();
            const float minConf = 0.4f;  // 與 ActionEvaluator 建構參數一致
            for (const auto& pr : kPairs) {
                if (kp.size() < 17) break;
                if (kp[pr[0]].confidence < minConf ||
                    kp[pr[1]].confidence < minConf)
                    continue;
                cv::line(dbg, cv::Point(kp[pr[0]].point) + off,
                         cv::Point(kp[pr[1]].point) + off, {255, 200, 0}, 2);
            }
            for (size_t i = 0; i < kp.size(); ++i) {
                const bool ok = kp[i].confidence >= minConf;
                const cv::Point p = cv::Point(kp[i].point) + off;
                cv::circle(dbg, p, 4, ok ? cv::Scalar(0, 255, 0)
                                         : cv::Scalar(0, 0, 255), -1);
                cv::putText(dbg, cv::format("%zu:%.2f", i, kp[i].confidence),
                            p + cv::Point(5, -5), cv::FONT_HERSHEY_SIMPLEX,
                            0.4, {255, 255, 255}, 1);
            }
        }



        std::lock_guard<std::mutex> lk(mtx_);
        debugFrame_ = std::move(dbg);
        EvalResult r = evaluator_.evaluate(kp);
        passed_ = r.valid && r.passed;
        if (hasAction_) records_.push_back({captured, passed_});

        EvalResult r1 = relaxEvaluator_.evaluate(kp);
        relaxFrames_ = (r1.passed && r1.valid) ? relaxFrames_ + 1 : 0;

        // DEBUG: 每幀印出狀態與推論耗時
        const float dbgMs = std::chrono::duration<float, std::milli>(
                                Clock::now() - captured).count();
        std::fprintf(stderr,
                     "[%d] rois=%zu kp=%zu valid=%d score=%.0f hasAction=%d "
                     "耗時=%.0fms\n",
                     ++dbgFrame, rois.size(), kp.size(), r.valid, r.score,
                     hasAction_, dbgMs);  // DEBUG
    }
}
