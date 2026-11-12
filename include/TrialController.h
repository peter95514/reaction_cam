#pragma once
#include <atomic>
#include <memory>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <thread>

#include "ActionEvaluator.h"
#include "MotionDetector.h"
#include "Screen.h"
#include "Cue.h"

struct FrameResult {
    Clock::time_point t;
    bool passed;
};

class TrialController {
private:
    cv::VideoCapture cap_;
    std::unique_ptr<MotionDetector> detector_;
    ActionEvaluator evaluator_;
    ActionEvaluator relaxEvaluator_;
    mutable std::mutex mtx_;

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::vector<FrameResult> records_;

    bool passed_ = 0;
    bool hasAction_ = 0;

    int relaxFrames_ = 0;

    void workerLoop();

    cv::Mat debugFrame_; //debug

public:
    TrialController();
    ~TrialController();

    bool isPassed() const;

    void start();
    void stop();

    void setAction(const Action& action);

    void resetRecords();
    std::vector<FrameResult> snapshotRecords() const;
    bool debugFrame(cv::Mat& out) const;  // DEBUG
                                          //
    void setRelaxPose(const Action& pose);
    bool isRelaxed(int minFrames);
};
