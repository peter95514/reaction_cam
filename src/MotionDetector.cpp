#include <MotionDetector.h>

MotionDetector::MotionDetector(int history, double varThreshold, bool detectShadows, double minArea) {
    bg_subtractor = cv::createBackgroundSubtractorMOG2(history, varThreshold, detectShadows);
    min_area = minArea;
    skeleton_net = cv::dnn::readNetFromONNX("yolov8n-pose.onnx");
    roi_net = cv::dnn::readNetFromONNX("yolov8n.onnx");
}

std::vector<cv::Rect> MotionDetector::get_rois(const cv::Mat &frame) {
    std::vector<cv::Rect> rois;

    float person_conf_threshold = 0.4f;
    float person_nms_threshold = 0.4f;
    const int PERSON_CLASS_ID = 0;

    int origW = frame.cols, origH = frame.rows;
    int targetW = 640, targetH = 640;
    cv::Size input_size(targetW, targetH);

    float scale = std::min((float)targetW / origW, (float)targetH / origH);
    int newH = static_cast<int>(origH * scale);
    int newW = static_cast<int>(origW * scale);

    cv::Mat resize;
    cv::resize(frame, resize, cv::Size(newW, newH));

    int padX = (targetW - newW) / 2;
    int padY = (targetH - newH) / 2;

    cv::Mat padded(targetH, targetW, frame.type(), cv::Scalar(114, 114, 114));
    resize.copyTo(padded(cv::Rect(padX, padY, newW, newH)));
    cv::Mat blob = cv::dnn::blobFromImage(padded, 1.0 / 255.0, input_size, cv::Scalar(0, 0, 0), true, false);

    roi_net.setInput(blob);
    cv::Mat output = roi_net.forward();

    cv::Mat data = output.reshape(1, output.size[1]);
    cv::Mat dataT;
    cv::transpose(data, dataT);

    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;

    for (int i = 0; i < dataT.rows; ++i) {
        cv::Mat scores = dataT.row(i).colRange(4, dataT.cols);
        cv::Point classIdPoint;
        double maxScore;
        cv::minMaxLoc(scores, nullptr, &maxScore, nullptr, &classIdPoint);

        if (maxScore < person_conf_threshold) continue;
        if (classIdPoint.x != PERSON_CLASS_ID) continue;

        float cx = dataT.at<float>(i, 0);
        float cy = dataT.at<float>(i, 1);
        float w = dataT.at<float>(i, 2);
        float h = dataT.at<float>(i, 3);

        float x0 = (cx - w / 2 - padX) / scale;
        float y0 = (cy - h / 2 - padY) / scale;
        float boxW = w / scale;
        float boxH = h / scale;
        cv::Rect r(static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(boxW), static_cast<int>(boxH));
        r &= cv::Rect(0, 0, origW, origH);

        if (r.area() <= 0) continue;

        boxes.push_back(r);
        confidences.push_back(static_cast<float>(maxScore));
    }

    std::vector<int> nms_indices;
    cv::dnn::NMSBoxes(boxes, confidences, person_conf_threshold, person_nms_threshold, nms_indices);

    for (int idx : nms_indices) {
        if (boxes[idx].area() >= min_area) {
            rois.push_back(boxes[idx]);
        }
    }

    return rois;
}

int MotionDetector::get_frame_counter() {
    return frame_counter;
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

    skeleton_net.setInput(blob);
    cv::Mat output = skeleton_net.forward();

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
    frame_counter = (frame_counter + 1) % 60;
}

void MotionDetector::tracking_skeleton(const cv::Mat &roiframe) {}

bool MotionDetector::is_tracking_skeleton() {
    return is_tracking_skeleton_or_not;
}

std::vector<cv::Point> MotionDetector::get_skeleton_point() {
    return now_skeleton_point;
}

MotionDetector::~MotionDetector() {}
