#pragma once
#include <atomic>
#include <memory>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <thread>

#include "ActionEvaluator.h"
#include "MotionDetector.h"
#include "TrialScreen.h"

struct FrameResult {
    Clock::time_point t;
    bool lpass;
    bool rpass;
};

class TrialController {
private:
    cv::VideoCapture cap_;
    std::unique_ptr<MotionDetector> detector_;
    ActionEvaluator leftEval_;
    ActionEvaluator rightEval_;
    mutable std::mutex mtx_;

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::vector<FrameResult> records_;

    bool lpass_;
    bool rpass_;

    void workerLoop();

public:
    TrialController();
    ~TrialController();

    bool isPassed(Side side) const;

    void start();
    void stop();

    void resetRecords();
    std::vector<FrameResult> snapshotRecords() const;
};
