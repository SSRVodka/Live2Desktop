#include <httplib.cpp/httplib.h>
#include <chrono>
#include <thread>

#include "utils/logger.h"

int main() {
    httplib::Server svr;

    svr.Post("/v1/chat/completions", [](const httplib::Request& req, httplib::Response& res) {
        stdLogger.Test("Mock server: receive client message");
        if (req.has_header("Authorization")) {
            std::string key = req.headers.find("Authorization")->second;
            if (!key.empty()) {
                res.set_content(R"({"valid": true, "choices": [{"message": {"content": "OK"}}]})", "application/json");
            } else res.status = 401;
        } else res.status = 401;
    });

    svr.Post("/v1/nokey", [](const httplib::Request& req, httplib::Response& res) {
        stdLogger.Test("Mock server: receive client message (not check API key)");
        res.set_content(R"({"valid": true, "choices": [{"message": {"content": "OK"}}]})", "application/json");
    });

    svr.Post("/v1/invalid", [](const httplib::Request& req, httplib::Response& res) {
        stdLogger.Test("Mock server: receive client message (use invalid response)");
        res.set_content(R"({"valid": true, "message": "OK"})", "application/json");
    });

    svr.Post("/v1/timeout", [](const httplib::Request& req, httplib::Response& res) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10 * 1000));
        stdLogger.Test("Mock server: receive client message (timeout 10s)");
        res.set_content(R"({"valid": true, "message": "OK"})", "application/json");
    });

    svr.Post("/v1/toolcall", [](const httplib::Request& req, httplib::Response& res) {
        stdLogger.Test("Mock server: receive client message (toolcall)");
        
        std::string resp = "[{"
            R"("function":{ "id": "test-id", "name":"test_echo", "arguments":{"param0":"test-param0-value"} })"
        "}]";
        // 简单验证 JSON 格式（检查是否以 { 或 [ 开头和以 } 或 ] 结尾）
        if ((!resp.empty() && resp.front() == '{' && resp.back() == '}') ||
            (!resp.empty() && resp.front() == '[' && resp.back() == ']')) {
            std::string response = R"({"valid": true, "choices": [{"finish_reason": "tool_calls", "message": {"tool_calls": )" + 
                                resp + 
                                R"(}}]})";
            res.set_content(response, "application/json");
        } else {
            res.status = 400;
            res.set_content("Invalid JSON format", "text/plain");
        }
    });

    svr.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        stdLogger.Exception("Mock server: unknown request");
        res.status = 404;
        res.set_content("Not Found", "text/plain");
    });

    stdLogger.Test("Mock server: listen on port 26565");
    svr.listen("0.0.0.0", 26565);

    return 0;
}