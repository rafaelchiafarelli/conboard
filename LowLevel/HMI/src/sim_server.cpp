#include "sim_server.hpp"
#include "json.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <mutex>
#include <sstream>
#include <thread>

namespace sim_server {

namespace {

std::mutex g_mutex;
std::vector<std::string> g_pending;

// Reads one HTTP/1.1 request off `fd` (headers, then a Content-Length body)
// and returns just the body. Not a general parser -- this server has
// exactly one route and no keep-alive, so the request line and headers
// besides Content-Length are read and discarded.
std::string readBody(int fd)
{
    std::string buf;
    char chunk[4096];
    size_t headerEnd = std::string::npos;
    while (headerEnd == std::string::npos) {
        ssize_t n = ::recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0) return "";
        buf.append(chunk, static_cast<size_t>(n));
        headerEnd = buf.find("\r\n\r\n");
        if (buf.size() > 65536) return "";  // not a real HTTP client, bail
    }
    const std::string headers = buf.substr(0, headerEnd);
    std::string body = buf.substr(headerEnd + 4);

    size_t contentLength = 0;
    std::istringstream hs(headers);
    std::string line;
    while (std::getline(hs, line)) {
        const std::string lower = [&] {
            std::string s = line;
            for (auto &ch : s) ch = static_cast<char>(::tolower(static_cast<unsigned char>(ch)));
            return s;
        }();
        if (lower.rfind("content-length:", 0) == 0) {
            try {
                contentLength = static_cast<size_t>(std::stoul(line.substr(line.find(':') + 1)));
            } catch (...) {
                contentLength = 0;
            }
        }
    }
    while (body.size() < contentLength) {
        ssize_t n = ::recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0) break;
        body.append(chunk, static_cast<size_t>(n));
    }
    return body;
}

void sendResponse(int fd, const std::string &status, const std::string &jsonBody)
{
    std::ostringstream resp;
    resp << "HTTP/1.1 " << status << "\r\n"
         << "Content-Type: application/json\r\n"
         << "Content-Length: " << jsonBody.size() << "\r\n"
         << "Connection: close\r\n\r\n"
         << jsonBody;
    const std::string out = resp.str();
    ::send(fd, out.data(), out.size(), 0);
}

void handleConn(int fd)
{
    const std::string body = readBody(fd);
    try {
        const auto j = nlohmann::json::parse(body);
        const std::string control = j.value("control", "");
        if (control.empty()) {
            sendResponse(fd, "400 Bad Request", R"({"error":"missing control field"})");
        } else {
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                g_pending.push_back(control);
            }
            sendResponse(fd, "200 OK", "{\"queued\":\"" + control + "\"}");
        }
    } catch (const std::exception &e) {
        sendResponse(fd, "400 Bad Request", std::string(R"({"error":"invalid JSON: )") + e.what() + "\"}");
    }
    ::close(fd);
}

void acceptLoop(int listenFd)
{
    for (;;) {
        int fd = ::accept(listenFd, nullptr, nullptr);
        if (fd < 0) continue;
        handleConn(fd);
    }
}

} // namespace

bool start(int port)
{
    const int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) return false;

    const int opt = 1;
    ::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(listenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        ::close(listenFd);
        return false;
    }
    if (::listen(listenFd, 8) < 0) {
        ::close(listenFd);
        return false;
    }

    std::thread(acceptLoop, listenFd).detach();
    return true;
}

std::vector<std::string> drain()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<std::string> out;
    out.swap(g_pending);
    return out;
}

} // namespace sim_server
