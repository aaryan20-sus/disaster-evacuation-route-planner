#include "HttpServer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

struct ClientContext {
    HttpServer* server;
    SOCKET client_socket;
};

void HttpResponse::set_content(const std::string& content, const std::string& content_type) {
    body = content;
    headers["Content-Type"] = content_type;
    headers["Content-Length"] = std::to_string(content.size());
}

void HttpResponse::set_status(int code, const std::string& text) {
    status_code = code;
    if (!text.empty()) {
        status_text = text;
    } else {
        switch (code) {
            case 200: status_text = "OK"; break;
            case 400: status_text = "Bad Request"; break;
            case 404: status_text = "Not Found"; break;
            case 405: status_text = "Method Not Allowed"; break;
            case 500: status_text = "Internal Server Error"; break;
            default: status_text = "Response"; break;
        }
    }
}

HttpServer::HttpServer() : host("127.0.0.1"), port(8080), listen_socket(INVALID_SOCKET), is_running(false) {
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        std::cerr << "[HttpServer] WSAStartup failed: " << res << std::endl;
    }
}

HttpServer::~HttpServer() {
    stop();
    WSACleanup();
}

void HttpServer::Get(const std::string& path, HttpHandler handler) {
    get_routes[path] = handler;
}

void HttpServer::Post(const std::string& path, HttpHandler handler) {
    post_routes[path] = handler;
}

void HttpServer::set_mount_point(const std::string& mount, const std::string& dir) {
    static_mount_path = mount;
    static_dir_path = dir;
    // Normalize path separators
    for (char& c : static_dir_path) {
        if (c == '/') c = '\\';
    }
}

std::string HttpServer::getMimeType(const std::string& path) {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) return "application/octet-stream";
    std::string ext = path.substr(dot_pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
    if (ext == ".css") return "text/css; charset=utf-8";
    if (ext == ".js") return "application/javascript; charset=utf-8";
    if (ext == ".json") return "application/json; charset=utf-8";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".txt") return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

void HttpServer::serveStaticFile(const std::string& rel_path, HttpResponse& res) {
    std::string clean_path = rel_path;
    if (clean_path.empty() || clean_path == "/") {
        clean_path = "/index.html";
    }

    std::string full_path = static_dir_path + clean_path;
    for (char& c : full_path) {
        if (c == '/') c = '\\';
    }

    std::ifstream file(full_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        res.set_status(404, "Not Found");
        res.set_content("<html><body><h1>404 Not Found</h1><p>Requested file was not located.</p></body></html>", "text/html");
        return;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string data = ss.str();
    res.set_status(200, "OK");
    res.set_content(data, getMimeType(full_path));
}

DWORD WINAPI HttpServer::ClientThreadHandler(LPVOID lpParam) {
    ClientContext* ctx = static_cast<ClientContext*>(lpParam);
    if (ctx && ctx->server) {
        ctx->server->handleClientConnection(ctx->client_socket);
    }
    delete ctx;
    return 0;
}

void HttpServer::handleClientConnection(SOCKET client_socket) {
    std::vector<char> buffer(8192);
    int bytes_received = recv(client_socket, buffer.data(), (int)buffer.size() - 1, 0);
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';

    std::string raw_request(buffer.data(), bytes_received);
    std::istringstream req_stream(raw_request);
    std::string line;

    HttpRequest req;
    HttpResponse res;

    // Set standard CORS headers for modern web frontends
    res.headers["Access-Control-Allow-Origin"] = "*";
    res.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS, PUT, DELETE";
    res.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization, Accept, X-Requested-With";

    // Read Request Line
    if (std::getline(req_stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream line_stream(line);
        std::string version;
        line_stream >> req.method >> req.path >> version;

        // Split query string if present
        size_t qmark = req.path.find('?');
        if (qmark != std::string::npos) {
            req.query_string = req.path.substr(qmark + 1);
            req.path = req.path.substr(0, qmark);
        }
    }

    // Read Headers
    int content_length = 0;
    while (std::getline(req_stream, line) && line != "\r" && !line.empty()) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string header_name = line.substr(0, colon);
            std::string header_val = line.substr(colon + 1);
            // Trim leading whitespace
            while (!header_val.empty() && header_val.front() == ' ') header_val.erase(0, 1);
            req.headers[header_name] = header_val;

            if (header_name == "Content-Length" || header_name == "content-length") {
                try {
                    content_length = std::stoi(header_val);
                } catch (...) {}
            }
        }
    }

    // Read Body if applicable
    if (content_length > 0) {
        size_t header_end = raw_request.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            size_t body_start = header_end + 4;
            if (body_start < raw_request.size()) {
                req.body = raw_request.substr(body_start);
            }
            int remaining = content_length - (int)req.body.size();
            while (remaining > 0) {
                int n = recv(client_socket, buffer.data(), std::min((int)buffer.size(), remaining), 0);
                if (n <= 0) break;
                req.body.append(buffer.data(), n);
                remaining -= n;
            }
        }
    }

    // Handle Pre-flight CORS OPTIONS request
    if (req.method == "OPTIONS") {
        res.set_status(200, "OK");
        res.set_content("", "text/plain");
    } else if (req.method == "GET") {
        auto it = get_routes.find(req.path);
        if (it != get_routes.end()) {
            it->second(req, res);
        } else if (!static_dir_path.empty()) {
            serveStaticFile(req.path, res);
        } else {
            res.set_status(404, "Not Found");
            res.set_content("{\"error\":\"Endpoint not found\"}", "application/json");
        }
    } else if (req.method == "POST") {
        auto it = post_routes.find(req.path);
        if (it != post_routes.end()) {
            it->second(req, res);
        } else {
            res.set_status(404, "Not Found");
            res.set_content("{\"error\":\"API route not found\"}", "application/json");
        }
    } else {
        res.set_status(405, "Method Not Allowed");
        res.set_content("{\"error\":\"Method Not Allowed\"}", "application/json");
    }

    // Construct raw response string
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 " << res.status_code << " " << res.status_text << "\r\n";
    for (const auto& h : res.headers) {
        response_stream << h.first << ": " << h.second << "\r\n";
    }
    response_stream << "Connection: close\r\n\r\n";
    response_stream << res.body;

    std::string out = response_stream.str();
    send(client_socket, out.c_str(), (int)out.size(), 0);
    closesocket(client_socket);
}

bool HttpServer::listen(const std::string& bind_host, int bind_port) {
    host = bind_host;
    port = bind_port;

    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        std::cerr << "[HttpServer] socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    // Allow quick port reuse
    int opt = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(bind_host.c_str());
    server_addr.sin_port = htons(bind_port);

    if (bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[HttpServer] bind failed on " << bind_host << ":" << bind_port 
                  << " with error: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        listen_socket = INVALID_SOCKET;
        return false;
    }

    if (::listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[HttpServer] listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        listen_socket = INVALID_SOCKET;
        return false;
    }

    is_running = true;
    std::cout << "\n======================================================\n";
    std::cout << "  DISASTER EVACUATION ROUTE PLANNER SERVER ACTIVE     \n";
    std::cout << "  Local URL: http://" << bind_host << ":" << bind_port << "\n";
    std::cout << "  Frontend:  http://localhost:" << bind_port << "/index.html\n";
    std::cout << "======================================================\n" << std::endl;

    while (is_running) {
        sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_socket = accept(listen_socket, (sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            if (!is_running) break;
            continue;
        }

        ClientContext* ctx = new ClientContext();
        ctx->server = this;
        ctx->client_socket = client_socket;

        HANDLE thread_handle = CreateThread(NULL, 0, ClientThreadHandler, ctx, 0, NULL);
        if (thread_handle) {
            CloseHandle(thread_handle);
        } else {
            // Fallback to synchronous execution if thread creation fails
            handleClientConnection(client_socket);
            delete ctx;
        }
    }

    return true;
}

void HttpServer::stop() {
    is_running = false;
    if (listen_socket != INVALID_SOCKET) {
        closesocket(listen_socket);
        listen_socket = INVALID_SOCKET;
    }
}