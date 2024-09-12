#include <gflags/gflags.h>
#include <glog/logging.h>
#include <seasocks/PrintfLogger.h>
#include <seasocks/Server.h>
#include <seasocks/StringUtil.h>
#include <seasocks/WebSocket.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <set>
#include <thread>
#include <string>

#include "apriltag_gpu.h"
#include "apriltag_utils.h"
#include "opencv2/opencv.hpp"

#include "DoubleArraySender.h"
#include "DoubleValueSender.h"
#include "IntegerValueSender.h"
#include "BooleanValueSender.h"

extern "C" {
#include "apriltag.h"
#include "apriltag_pose.h"
#include "common/zarray.h"
}

using json = nlohmann::json;
using namespace std;

DEFINE_int32(camera_idx, 0, "Camera index");

enum ExposureMode { AUTO = 0, MANUAL = 1 };

class AprilTagHandler : public seasocks::WebSocket::Handler {
 public:
  AprilTagHandler(std::shared_ptr<seasocks::Server> server) : server_(server) {}

  void onConnect(seasocks::WebSocket* socket) override {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.insert(socket);
  }

  void onDisconnect(seasocks::WebSocket* socket) override {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.erase(socket);
  }

  void onData(seasocks::WebSocket* socket, const char* data) override {
    try {
      std::cerr << "Received data: " << data << std::endl;
      auto j = json::parse(data);
      settings_changed_ = false;
      if (j["type"] == "brightness") {
        brightness_ = j["value"].get<int>();
        settings_changed_ = true;
      }
      if (j["type"] == "exposure") {
        exposure_ = j["value"].get<int>();
        settings_changed_ = true;
      }
      if (j["type"] == "exposure-mode") {
        exposure_mode_ = j["value"].get<int>();
        settings_changed_ = true;
      }

    } catch (const json::parse_error& e) {
      LOG(ERROR) << "JSON parse error: " << e.what();
    }
  }

  void broadcast(const std::vector<uint8_t>& data) {
    server_->execute([this, data] {
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto client : clients_) {
        client->send(data.data(), data.size());
      }
    });
  }

  void startReadAndSendThread(const int camera_idx) {
    read_thread_ = std::thread(&AprilTagHandler::readAndSend, this, camera_idx);
  }

  void joinReadAndSendThread() {
    if (read_thread_.joinable()) {
      read_thread_.join();
    }
  }

  void readAndSend(const int camera_idx) {
    LOG(INFO) << "Enabling video capture";
    cv::VideoCapture cap(camera_idx, cv::CAP_V4L);
    if (!cap.isOpened()) {
      LOG(ERROR) << "Couldn't open video capture device";
      return;
    }
    cap.set(cv::CAP_PROP_CONVERT_RGB, false);
    // cap.set(CAP_PROP_MODE, CV_CAP_MODE_YUYV);
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

    int frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    int frame_rate = cap.get(cv::CAP_PROP_FPS);

    std::cout << "  " << frame_width << "x" << frame_height << " @"
              << frame_rate << "FPS" << std::endl;

    std::cout << "AUTO Exposure: " << cap.get(cv::CAP_PROP_AUTO_EXPOSURE)
              << std::endl;
    std::cout << "Brightness: " << cap.get(cv::CAP_PROP_BRIGHTNESS)
              << std::endl;
    std::cout << "Contrast: " << cap.get(cv::CAP_PROP_CONTRAST) << std::endl;

    // Setup the apriltag detector.
    apriltag_family_t* tf = nullptr;
    apriltag_detector_t* td = nullptr;
    const char* tag_family = "tag36h11";
    frc971::apriltag::CameraMatrix cam;
    frc971::apriltag::DistCoeffs dist;

    setup_tag_family(&tf, tag_family);
    td = apriltag_detector_create();
    apriltag_detector_add_family(td, tf);

    td->quad_decimate = 2.0;
    td->quad_sigma = 0.0;
    td->nthreads = 1;
    td->debug = false;
    td->refine_edges = true;
    td->wp = workerpool_create(1);

    // Setup Camera Matrix
    cam.fx = 905.495617;
    cam.fy = 907.909470;
    cam.cx = 609.916016;
    cam.cy = 352.682645;

    // Setup Distortion Coefficients
    dist.k1 = 0.059238;
    dist.k2 = -0.075154;
    dist.p1 = -0.003801;
    dist.p2 = 0.001113;
    dist.k3 = 0.0;

    // Setup the detection info struct for use down below.
    apriltag_detection_info_t info;
    info.tagsize =
        0.175;  // Measured in meters, with a ruler, for tag family 36h11
    info.fx = cam.fx;
    info.fy = cam.fy;
    info.cx = cam.cx;
    info.cy = cam.cy;

    frc971::apriltag::GpuDetector detector(frame_width, frame_height, td, cam,
                                           dist);

    cv::Mat bgr_img, yuyv_img;
    while (running_) {
      // Handle settings changes.
      if (settings_changed_.exchange(false)) {
        std::cout << "Setting changed" << std::endl;
        if (exposure_mode_ == 0) {
          std::cout << "Auto Exposure set to Auto" << std::endl;
          cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 3);
        } else if (exposure_mode_ == 1) {
          std::cout << "Auto Exposure set to Manual" << std::endl;
          cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
          cap.set(cv::CAP_PROP_BRIGHTNESS, brightness_);
          cap.set(cv::CAP_PROP_EXPOSURE, exposure_);
        }
      }

      cap >> yuyv_img;
      cv::cvtColor(yuyv_img, bgr_img, cv::COLOR_YUV2BGR_YUYV);

      detector.Detect(yuyv_img.data);
      const zarray_t* detections = detector.Detections();
      draw_detection_outlines(bgr_img, const_cast<zarray_t*>(detections));

      // Encode the image to JPEG
      std::vector<uchar> buffer;
      cv::imencode(".jpg", bgr_img, buffer);

      // Broadcast the image
      broadcast(buffer);

      // Determine the pose of the tags.
      if (zarray_size(detections) > 0) {
        json detections_record;
        detections_record["Detections"] = json::array();

        for (int i = 0; i < zarray_size(detections); i++) {
          json detection_record;

          apriltag_detection_t* det;
          zarray_get(const_cast<zarray_t*>(detections), i, &det);

          // Setup the detection info struct for use down below.
          info.det = det;

          apriltag_pose_t pose;
          double err = estimate_tag_pose(&info, &pose);

          matd_print(pose.R, "%.3f ");
          matd_print(pose.t, "%.3f ");
          std::cout << "Pose Error: " << err << std::endl;

          detection_record["id"] = det->id;
          tagIDSender_.sendValue(det->id);
          detection_record["hamming"] = det->hamming;
          detection_record["pose_error"] = err;

          // TODO: store pose in the json record.
        }
      }
    }

    // Clean up
    apriltag_detector_destroy(td);
    teardown_tag_family(&tf, tag_family);
  }

  void stop() { running_ = false; }

 private:
  IntegerValueSender tagIDSender_{"tag_id"};
  BooleanValueSender isConnectedSender_{"ORIN_CONNECTED"};
  std::set<seasocks::WebSocket*> clients_;
  std::mutex mutex_;
  std::shared_ptr<seasocks::Server> server_;
  std::atomic<bool> running_{true};
  std::atomic<int> brightness_{50};
  std::atomic<int> exposure_{50};
  std::atomic<int> exposure_mode_{0};
  std::atomic<bool> settings_changed_{false};
  std::thread read_thread_;
};

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::SetVLOGLevel("*", FLAGS_v);

  auto logger = std::make_shared<seasocks::PrintfLogger>();
  auto server = std::make_shared<seasocks::Server>(logger);

  //isConnectedSender_.setDefaultValue(false);
  try {
    auto handler = std::make_shared<AprilTagHandler>(server);
    server->addWebSocketHandler("/ws", handler);

    handler->startReadAndSendThread(FLAGS_camera_idx);
    server->serve("", 8080);
    handler->stop();
    handler->joinReadAndSendThread();
    //isConnectedSender_.sendValue(true); //Is this the right place to put this?
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what();
    return 1;
  }

  gflags::ShutDownCommandLineFlags();

  return 0;
}
