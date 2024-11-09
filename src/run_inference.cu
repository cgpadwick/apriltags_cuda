#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <NvInfer.h>
#include <cuda_runtime_api.h>
#include "cameraexception.h"
#include "opencv2/opencv.hpp"

// Logger for TensorRT info/warning/errors
class Logger : public nvinfer1::ILogger {
    void log(Severity severity, const char* msg) noexcept override {
        if (severity != Severity::kINFO) {
            std::cout << msg << std::endl;
        }
    }
} gLogger;

// Helper function to check CUDA errors
#define CHECK_CUDA(call) \
    do { \
        cudaError_t status = call; \
        if (status != cudaSuccess) { \
            std::cerr << "CUDA error at line " << __LINE__ << ": " \
                      << cudaGetErrorString(status) << std::endl; \
            return false; \
        } \
    } while(0)

class TensorRTInference {
public:
    TensorRTInference(const std::string& enginePath) 
        : mEngine(nullptr), mContext(nullptr) {
        initializeEngine(enginePath);
    }

    ~TensorRTInference() {
        if (mContext) mContext->destroy();
        if (mEngine) mEngine->destroy();
        if (mRuntime) mRuntime->destroy();
        for (void* buf : mDeviceBuffers) {
            cudaFree(buf);
        }
        if (mStream) cudaStreamDestroy(mStream);
    }

    bool initializeEngine(const std::string& enginePath) {
        // Read engine file
        std::ifstream file(enginePath, std::ios::binary);
        if (!file.good()) {
            std::cerr << "Error opening engine file: " << enginePath << std::endl;
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> engineData(size);
        file.read(engineData.data(), size);
        file.close();

        // Create runtime and engine
        mRuntime = nvinfer1::createInferRuntime(gLogger);
        if (!mRuntime) {
            std::cerr << "Error creating TensorRT runtime" << std::endl;
            return false;
        }

        mEngine = mRuntime->deserializeCudaEngine(engineData.data(), size);
        if (!mEngine) {
            std::cerr << "Error deserializing CUDA engine" << std::endl;
            return false;
        }

        mContext = mEngine->createExecutionContext();
        if (!mContext) {
            std::cerr << "Error creating execution context" << std::endl;
            return false;
        }

        // Create CUDA stream
        CHECK_CUDA(cudaStreamCreate(&mStream));

        // Allocate device buffers
        for (int i = 0; i < mEngine->getNbBindings(); i++) {
            nvinfer1::Dims dims = mEngine->getBindingDimensions(i);
            size_t size = 1;
            for (int j = 0; j < dims.nbDims; j++) {
                size *= dims.d[j];
            }
            size *= sizeof(float);  // Assuming float32 data type

            void* deviceBuffer;
            CHECK_CUDA(cudaMalloc(&deviceBuffer, size));
            mDeviceBuffers.push_back(deviceBuffer);
            
            if (mEngine->bindingIsInput(i)) {
                mInputSize = size;
                mInputDims = dims;
            } else {
                mOutputSize = size;
                mOutputDims = dims;
            }
        }

        return true;
    }

    bool infer(const float* input, float* output) {
        // Copy input to device
        CHECK_CUDA(cudaMemcpyAsync(
            mDeviceBuffers[0], input, mInputSize,
            cudaMemcpyHostToDevice, mStream
        ));

        // Execute inference
        if (!mContext->enqueueV2(mDeviceBuffers.data(), mStream, nullptr)) {
            std::cerr << "Error running inference" << std::endl;
            return false;
        }

        // Copy output back to host
        CHECK_CUDA(cudaMemcpyAsync(
            output, mDeviceBuffers[1], mOutputSize,
            cudaMemcpyDeviceToHost, mStream
        ));

        // Synchronize stream
        CHECK_CUDA(cudaStreamSynchronize(mStream));

        return true;
    }

    nvinfer1::Dims getInputDims() const { return mInputDims; }
    nvinfer1::Dims getOutputDims() const { return mOutputDims; }
    size_t getInputSize() const { return mInputSize; }
    size_t getOutputSize() const { return mOutputSize; }

private:
    nvinfer1::IRuntime* mRuntime;
    nvinfer1::ICudaEngine* mEngine;
    nvinfer1::IExecutionContext* mContext;
    cudaStream_t mStream;
    std::vector<void*> mDeviceBuffers;
    size_t mInputSize;
    size_t mOutputSize;
    nvinfer1::Dims mInputDims;
    nvinfer1::Dims mOutputDims;
};

void printCameraSettings(const cv::VideoCapture& cap) {
    int frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    int frame_rate = cap.get(cv::CAP_PROP_FPS);

    std::cout << "  " << frame_width << "x" << frame_height << " @"
              << frame_rate << "FPS" << std::endl;

    std::cout << " format is: " << cap.get(cv::CAP_PROP_FORMAT) << std::endl;

    std::cout << "AUTO Exposure: " << cap.get(cv::CAP_PROP_AUTO_EXPOSURE)
              << std::endl;
    std::cout << "Brightness: " << cap.get(cv::CAP_PROP_BRIGHTNESS)
              << std::endl;
    std::cout << "Contrast: " << cap.get(cv::CAP_PROP_CONTRAST) << std::endl;
  }

// Example usage
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <engine_file>" << std::endl;
        return 1;
    }

    // Initialize inference engine
    TensorRTInference inference(argv[1]);

    // Get input dimensions
    auto inputDims = inference.getInputDims();
    size_t inputElements = 1;
    for (int i = 0; i < inputDims.nbDims; i++) {
        std::cout << inputDims.d[i] << std::endl;
        inputElements *= inputDims.d[i];
    }

    std::cout << inputDims.nbDims << ", " << inputElements << std::endl;

    // Prepare input data (example with random data)
    std::vector<float> input(inputElements);
    for (size_t i = 0; i < inputElements; i++) {
        input[i] = static_cast<float>(rand()) / RAND_MAX;
    }

    cv::VideoCapture cap;
    bool camera_started = false;
    while (!camera_started) {
      try {
        cap.open(0, cv::CAP_V4L);
        if (cap.isOpened()) {
          camera_started = true;
          std::cout << "Camera started successfully on index " << 0
                    << std::endl;
        } else {
          throw CameraException();
        }
      } catch (const CameraException& e) {
        std::cout << "Couldn't open video capture device: " << e.what()
                  << std::endl;
        std::cout << "Retrying in 1 second ...";
      }
    }

    // Set video mode, resolution and frame rate.
    int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    cap.set(cv::CAP_PROP_FOURCC, fourcc);
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 800);
    cap.set(cv::CAP_PROP_FPS, 30);
    cap.set(cv::CAP_PROP_CONVERT_RGB, true);

    printCameraSettings(cap);

    float mean[3] = {0.485f, 0.456f, 0.406f};       // ImageNet means
    float std[3]  = {0.229f, 0.224f, 0.225f};       // ImageNet std devs

    cv::Mat bgr_img, rgb_img, rgb_img_subsampled;
    int w = 640;
    int h = 640;
    while (1) {
        cap >> bgr_img;

        // Preprocess the data before putting it through the engine.
        cv::cvtColor(bgr_img, rgb_img, cv::COLOR_BGR2RGB);
        cv::resize(rgb_img, rgb_img_subsampled, cv::Size(h, w), 0, 0, cv::INTER_LINEAR);
        rgb_img_subsampled.convertTo(rgb_img_subsampled, CV_32F);
        rgb_img_subsampled /= 255.0f;

        // normalize by mean and std dev.
        std::vector<cv::Mat> channels;
        cv::split(rgb_img_subsampled, channels);
        for (int i=0; i < 3; ++i) {
            channels[i] = (channels[i] - mean[i]) / std[i];
        }

        std::vector<float> input_tensor(1 * 3 * h * w);
        float* input_buffer = input_tensor.data();
        for (int c=0; c<3; c++) {
            int channel_offset = c * h * w;
            float* channel_data = reinterpret_cast<float*>(channels[c].data);
            std::memcpy(input_buffer + channel_offset, 
                       channel_data, 
                       h * w * sizeof(float));
        }

        // Prepare output buffer
        std::cout << "outputsize: " << inference.getOutputSize() << std::endl;
        std::vector<float> output(inference.getOutputSize() / sizeof(float));

        // Run inference
        if (!inference.infer(input.data(), output.data())) {
            std::cerr << "Inference failed" << std::endl;
            return 1;
        }

        // Print first few outputs
        std::cout << "Output preview: ";
        for (int i = 0; i < std::min(5ul, output.size()); i++) {
            std::cout << output[i] << " ";
        }
        std::cout << std::endl;

    }

    

    
    

    return 0;
}
