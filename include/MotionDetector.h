#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

struct Keypoint {
    cv::Point2f point;
    float confidence;
};

class MotionDetector {
private:
    cv::dnn::Net skeleton_net;
    cv::dnn::Net roi_net;

    cv::Ptr<cv::BackgroundSubtractorMOG2> bg_subtractor;
    double min_area;

    bool is_tracking_skeleton_or_not;
    std::vector<cv::Point> now_skeleton_point;
    cv::Rect skeleton_bbox;
    std::vector<Keypoint> now_keypoints;
    int frame_counter = 0;

public:
    MotionDetector(int history = 500, double varThreshold = 25.0,
                   bool detectShadows = false, double minArea = 500);
    std::vector<cv::Rect> get_rois(const cv::Mat& frame);
    bool is_tracking_skeleton();
    int get_frame_counter();
    void update(const cv::Mat& roiframe);
    void detect_skeleton(const cv::Mat& roiframe);
    void tracking_skeleton(const cv::Mat& roiframe);
    cv::Rect get_skeleton();
    std::vector<cv::Point> get_skeleton_point();
    std::vector<Keypoint> get_keypoints();
    ~MotionDetector();
};
