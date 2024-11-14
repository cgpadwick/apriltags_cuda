#ifndef YOLO_DETECTION_H_
#define YOLO_DETECTION_H_

#include <iostream>

class YoloDetection {
 public:
  YoloDetection(const float& x, const float& y, const float& w, const float& h,
                const float& conf, const int& cls)
      : x_(x),
        y_(y),
        w_(w),
        h_(h),
        conf_(conf),
        cls_(cls){

        };

  YoloDetection(const float* det)
      : x_(det[0]),
        y_(det[1]),
        w_(det[2]),
        h_(det[3]),
        conf_(det[4]),
        cls_(int(det[5])){

        };

  ~YoloDetection(){};

  YoloDetection(const YoloDetection& det) {
    x_ = det.x_;
    y_ = det.y_;
    w_ = det.w_;
    h_ = det.h_;
    conf_ = det.conf_;
    cls_ = det.cls_;
  };

  friend std::ostream& operator<<(std::ostream& os, const YoloDetection& d) {
    os << "x: " << d.x_ << " y: " << d.y_ << " w: " << d.w_ << " h: " << d.h_
       << " conf: " << d.conf_ << " class: " << d.cls_ << std::endl;
    return os;
  }

  float conf() { return conf_; };

 private:
  float x_;
  float y_;
  float w_;
  float h_;
  float conf_;
  int cls_;
};

void analyze_detections(const std::vector<float>& data,
                        const float thresh = 0.8) {
  int num_det = data.size() / 6;
  const float* buf = data.data();
  std::vector<YoloDetection> keeps;
  keeps.reserve(num_det);
  for (int ii = 0; ii < num_det; ++ii) {
    YoloDetection det(buf);
    if (det.conf() >= thresh) {
      // std::cout << "kept a detection " << det.conf() << std::endl;
      keeps.push_back(det);
    }
    // std::cout << det;
    buf += 6;
  }
  std::cout << "Kept " << keeps.size() << " out of " << num_det
            << " detections." << std::endl;

  for (int ii = 0; ii < keeps.size(); ++ii) {
    std::cout << keeps[ii];
  }
}

#endif