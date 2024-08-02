#include <App.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "base64.h"

// Function to read a file into a string
std::string read_file(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

struct PerSocketData {
	/* Fill with user data */
	std::vector<uint8_t> data;
};

int main() {
    // Create a uWebSockets app
    uWS::App().ws<PerSocketData>("/ws", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024 * 1024,
        .idleTimeout = 16,

        /* Handlers */
        // Handler called when a new WebSocket connection is established
        .open = [](auto* ws) {
            std::cout << "Client connected" << std::endl;
        },
        // Handler called when a message is received from a client
        .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
            std::vector<uint8_t> data(message.begin(), message.end());
            std::string base64Image = base64_encode(data.data(), data.size());
            ws->send(base64Image, uWS::OpCode::TEXT);
        },
        // Handler called when a WebSocket connection is closed
        .close = [](auto* ws, int code, std::string_view message) {
            std::cout << "Client disconnected" << std::endl;
        }
    })
    // Handler for HTTP GET requests to any path
    .get("/*", [](auto* res, auto* req) {
        std::string html = read_file("web/index.html");
        res->writeHeader("Content-Type", "text/html")->end(html);
    })
    // Listen on port 9090
    .listen(9090, [](auto* token) {
        if (token) {
            std::cout << "Server running on port 9090" << std::endl;
        } else {
            std::cerr << "Failed to listen on port 9090" << std::endl;
        }
    })
    // Run the app
    .run();

    return 0;
}
