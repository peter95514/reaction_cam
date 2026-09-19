#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "MotionDetector.h"

enum Joint {
    NOSE,
    L_EYE,
    R_EYE,
    L_EAR,
    R_EAR,
    L_SHOULDER,
    R_SHOULDER,
    L_ELBOW,
    R_ELBOW,
    L_WRIST,
    R_WRIST,
    L_HIP,
    R_HIP,
    L_KNEE,
    R_KNEE,
    L_ANKLE,
    R_ANKLE
};

struct AngleRule {
    std::string name;
    int a, b, c;
    float minDeg, maxDeg;
    float weight = 1.0f;
};

struct RuleResult {
    std::string name;
    bool valid = false;
    bool inRange = false;
    float angle = 0.f;
};

struct EvalResult {
    bool valid =
        false;  // false = 可見的點太少，這幀不該拿來判斷 (不是「動作錯」)
    bool passed = false;
    float score = 0.f;  // 0~100
    std::vector<std::string> feedback;
    std::vector<RuleResult> details;
};

class ActionEvaluator {
private:
    std::vector<AngleRule> rules_;
    float min_conf_;
    float pass_score_;
    float min_valid_ratio_;

public:
    ActionEvaluator(float min_conf, float pass_score, float min_valid_ratio);
    EvalResult evaluate(const std::vector<Keypoint>& kpts);
    static bool computeAngle(const std::vector<Keypoint>& k, int a, int b,
                             int c, float minConf, float& outDeg);
    void changerule(std::vector<AngleRule> rules);
};
