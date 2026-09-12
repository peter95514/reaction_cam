#include <MotionDetector.h>

#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/dnn/dnn.hpp>

MotionDetector::MotionDetector(int history, double varThreshold, bool detectShadows, double minArea) {
    bg_subtractor = cv::createBackgroundSubtractorMOG2(history, varThreshold, detectShadows);
    min_area = minArea;
    net = cv::dnn::readNetFromONNX("yolov8n-pose.onnx");
}

std::vector<cv::Rect> MotionDetector::get_rois(const cv::Mat &frame) {
    std::vector<cv::Rect> rois;
    cv::Mat gray, blurred, fg_mask, dilated;

    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);

    bg_subtractor->apply(blurred, fg_mask);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::dilate(fg_mask, dilated, kernel);

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(dilated, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
        if (cv::contourArea(contours[i]) >= min_area) {
            rois.push_back(cv::boundingRect(contours[i]));
        }
    }

    return rois;
}

void MotionDetector::detect_skeleton(const cv::Mat &roiframe) {
    int origW = roiframe.cols;
    int origH = roiframe.rows;
    int targetW = 640, targetH = 640;
    float confThreshold = 0;
    cv::Size input_size(640, 640);
    std::vector<cv::Point> result;

    float scale = std::min((float)targetW / origW, (float)targetH / origH);
    int newW = static_cast<int>(origW * scale);
    int newH = static_cast<int>(origH * scale);

    cv::Mat resized;
    cv::resize(roiframe, resized, cv::Size(newW, newH));

    int padX = (targetW - newW) / 2;
    int padY = (targetH - newH) / 2;

    cv::Mat padded(targetH, targetW, roiframe.type(), cv::Scalar(114, 114, 114));
    resized.copyTo(padded(cv::Rect(padX, padY, newW, newH)));

    cv::Mat blob = cv::dnn::blobFromImage(padded, 1.0 / 255.0, input_size, cv::Scalar(0, 0, 0), true, false);

    net.setInput(blob);
    cv::Mat output = net.forward();

    cv::Mat data = output.reshape(1, 56);
    cv::Mat dataT;
    cv::transpose(data, dataT);

    float bestconf = 0.0f;
    int bestidx = -1;

    for (int i = 0; i < dataT.rows; ++i) {
        float conf = dataT.at<float>(i, 4);
        if (conf > bestconf) {
            bestconf = conf;
            bestidx = i;
        }
    }

    std::vector<Keypoint> keypoints;
    const int numkpts = 17;
    for (int i = 0; i < numkpts; ++i) {
        float x = dataT.at<float>(bestidx, 5 + i * 3 + 0);
        float y = dataT.at<float>(bestidx, 5 + i * 3 + 1);
        float iconf = dataT.at<float>(bestidx, 5 + i * 3 + 2);
        keypoints.push_back({cv::Point2f(x, y), iconf});
    }

    result.reserve(keypoints.size());

    for (const auto &kp : keypoints) {
        if (kp.confidence < confThreshold) {
            result.emplace_back(-1, -1);
            continue;
        }
        float origX = (kp.point.x - padX) / scale;
        float origY = (kp.point.y - padY) / scale;
        result.emplace_back(static_cast<int>(origX), static_cast<int>(origY));
    }

    now_skeleton_point = result;
}

void MotionDetector::update(const cv::Mat &roiframe) {
    if (is_tracking_skeleton_or_not) {
        tracking_skeleton(roiframe);
    } else {
        detect_skeleton(roiframe);
    }
}

void MotionDetector::tracking_skeleton(const cv::Mat &roiframe) {}

bool MotionDetector::is_tracking_skeleton() {
    return is_tracking_skeleton_or_not;
}

std::vector<cv::Point> MotionDetector::get_skeleton_point() {
    return now_skeleton_point;
}

MotionDetector::~MotionDetector() {}
