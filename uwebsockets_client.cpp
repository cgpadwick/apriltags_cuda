#include <App.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;

struct UserData {
	/* Fill with user data */
	std::vector<uint8_t> data;
};


class WebSocketClient {
public:
    WebSocketClient(const std::string& url) : url(url) {}

    void run() {
        uWS::App().connect(url, {
            /* Settings */
            .compression = uWS::SHARED_COMPRESSOR,
            .maxPayloadLength = 16 * 1024 * 1024,
            .idleTimeout = 16,

            /* Handlers */
            .open = [this](uWS::WebSocket<false, true, UserData>* ws) {
                std::cout << "Connected to server" << std::endl;
                this->ws = ws;
                // Read and send the image once
                this->sendImage();
            },
            .message = [](uWS::WebSocket<false, true, UserData>* ws, std::string_view message, uWS::OpCode opCode) {
                std::cout << "Received message from server: " << message << std::endl;
            },
            .close = [this](uWS::WebSocket<false, true, UserData>* ws, int code, std::string_view message) {
                std::cout << "Disconnected from server" << std::endl;
            }
        }).run();
    }

private:
    void sendImage() {
        // Read image from disk
        Mat img = imread("data/colorimage.jpg", IMREAD_COLOR);
        if (img.empty()) {
            std::cerr << "Could not read the image: image.jpg" << std::endl;
            return;
        }

        // Encode the frame as JPEG
        std::vector<uchar> buf;
        imencode(".jpg", img, buf);

        // Send the JPEG-encoded frame over the WebSocket connection
        if (ws) {
            ws->send(reinterpret_cast<const char*>(buf.data()), buf.size(), uWS::BINARY);
        }
    }

    std::string url;
    uWS::WebSocket<false, true, UserData>* ws = nullptr;
};

int main() {
    std::string url = "ws://localhost:9090/ws";
    WebSocketClient client(url);
    client.run();
    return 0;
}
