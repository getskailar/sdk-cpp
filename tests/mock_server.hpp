#ifndef SKAILAR_TESTS_MOCK_SERVER_HPP
#define SKAILAR_TESTS_MOCK_SERVER_HPP

#include <chrono>
#include <functional>
#include <string>
#include <thread>

#include <httplib.h>

namespace skailar::testing {

/// A real loopback HTTP server for tests, the C++ analogue of Go's
/// httptest.NewServer. It binds to a random free port on 127.0.0.1 and serves on
/// a background thread until destroyed.
class MockServer {
public:
    /// Type of a route handler.
    using Handler = std::function<void(const httplib::Request&, httplib::Response&)>;

    MockServer() {
        port_ = server_.bind_to_any_port("127.0.0.1");
        thread_ = std::thread([this] { server_.listen_after_bind(); });
        wait_until_ready();
    }

    ~MockServer() {
        server_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    MockServer(const MockServer&) = delete;
    MockServer& operator=(const MockServer&) = delete;

    /// Base URL of the running server, for example "http://127.0.0.1:54321".
    std::string base_url() const { return "http://127.0.0.1:" + std::to_string(port_); }

    /// Registers a handler for an exact method and path.
    void on(const std::string& method, const std::string& path, Handler handler) {
        if (method == "GET") {
            server_.Get(path, std::move(handler));
        } else if (method == "POST") {
            server_.Post(path, std::move(handler));
        } else if (method == "PUT") {
            server_.Put(path, std::move(handler));
        } else if (method == "DELETE") {
            server_.Delete(path, std::move(handler));
        }
    }

private:
    void wait_until_ready() {
        for (int i = 0; i < 200; ++i) {
            if (server_.is_running()) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    httplib::Server server_;
    std::thread thread_;
    int port_ = 0;
};

} // namespace skailar::testing

#endif // SKAILAR_TESTS_MOCK_SERVER_HPP
