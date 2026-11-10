#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>

/**
 * Lightweight Zero-Dependency Native Windows HTTP Server
 * Engineered for MinGW / Windows without requiring POSIX pthread.
 * Supports:
 * - REST API Endpoints (GET, POST, OPTIONS)
 * - Automatic CORS Headers for browser compatibility
 * - Static file serving (HTML, CSS, JS, SVG, JSON)
 * - Multi-threaded connection processing via Win32 CreateThread
 */

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query_string;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    void set_content(const std::string& content, const std::string& content_type);
    void set_status(int code, const std::string& text = "");
};

typedef std::function<void(const HttpRequest&, HttpResponse&)> HttpHandler;

class HttpServer {
private:
    std::string host;
    int port;
    SOCKET listen_socket;
    bool is_running;
    std::string static_mount_path;
    std::string static_dir_path;

    std::unordered_map<std::string, HttpHandler> get_routes;
    std::unordered_map<std::string, HttpHandler> post_routes;

    static DWORD WINAPI ClientThreadHandler(LPVOID lpParam);
    void handleClientConnection(SOCKET client_socket);
    void serveStaticFile(const std::string& rel_path, HttpResponse& res);
    static std::string getMimeType(const std::string& filepath);

public:
    HttpServer();
    ~HttpServer();

    void Get(const std::string& path, HttpHandler handler);
    void Post(const std::string& path, HttpHandler handler);
    void set_mount_point(const std::string& mount, const std::string& dir);

    bool listen(const std::string& bind_host, int bind_port);
    void stop();
};

#endif // HTTP_SERVER_H