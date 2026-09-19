#include <ActionEvaluator.h>

#include <algorithm>
#include <cmath>

#include "MotionDetector.h"

ActionEvaluator::ActionEvaluator(float min_conf, float pass_score,
                                 float min_valid_ratio)
    : min_conf_(min_conf),
      pass_score_(pass_score),
      min_valid_ratio_(min_valid_ratio) {}

bool ActionEvaluator::computeAngle(const std::vector<Keypoint>& k, int a, int b,
                                   int c, float minConf, float& outDeg) {
    if (a < 0 || b < 0 || c < 0 || a >= (int)k.size() || b >= (int)k.size() ||
        c >= (int)k.size())
        return false;

    if (k[a].confidence < minConf || k[b].confidence < minConf ||
        k[c].confidence < minConf)
        return false;

    cv::Point2f v1 = k[a].point - k[b].point;
    cv::Point2f v2 = k[c].point - k[b].point;
    float n1 = std::hypot(v1.x, v1.y);
    float n2 = std::hypot(v2.x, v2.y);
    if (n1 < 1e-3f || n2 < 1e-3f) return false;

    float cosv = std::clamp(v1.dot(v2) / (n1 * n2), -1.f, 1.f);
    outDeg = std::acos(cosv) * 180.f / static_cast<float>(CV_PI);

    return true;
}

EvalResult ActionEvaluator::evaluate(const std::vector<Keypoint>& kpts) {
    EvalResult res;

    if (kpts.size() < 17 || rules_.empty()) return res;

    float allweight = 0.f;
    float validweight = 0.f;
    float gained = 0.f;

    for (const auto& r : rules_) {
        allweight += r.weight;

        RuleResult rr;
        rr.name = r.name;

        float deg = 0.f;
        if (!computeAngle(kpts, r.a, r.b, r.c, min_conf_, deg)) {
            res.details.push_back(rr);
            continue;
        }

        validweight += r.weight;
        rr.valid = 1;
        rr.angle = deg;

        if (deg >= r.minDeg && deg <= r.maxDeg) {
            rr.inRange = true;
            gained += r.weight;
        }
        res.details.push_back(rr);
    }
    if (allweight <= 0.f || validweight / allweight < min_valid_ratio_) {
        res.feedback.clear();
        return res;  // valid = false
    }

    res.valid = true;
    res.score = gained / validweight * 100.f;
    res.passed = res.score >= pass_score_;
    return res;
}

void ActionEvaluator::changerule(std::vector<AngleRule> rules) {
    rules_ = std::move(rules);
}
