#include <iostream>
#include <fstream>
#include "Graph.h"
#include "Algorithms.h"
#include "HttpServer.h"
#include "api.h"

bool fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "=================================================================\n";
    std::cout << "      INTELLIGENT DISASTER EVACUATION ROUTE PLANNER              \n";
    std::cout << "      ADSA Individual Project - B.Tech CSE (3rd Semester)        \n";
    std::cout << "=================================================================\n\n";

    int port = 8080;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            port = 8080;
        }
    }

    // Dynamic path resolution for flexible execution from root or bin/
    std::string data_file = "data/city_data.json";
    if (!fileExists(data_file)) {
        data_file = "../data/city_data.json";
    }

    std::string frontend_dir = "frontend";
    if (!fileExists(frontend_dir + "/index.html")) {
        frontend_dir = "../frontend";
    }

    std::cout << "[Init] Loading city network model from: " << data_file << std::endl;
    CityGraph graph;
    if (!graph.loadFromJsonFile(data_file)) {
        std::cerr << "[Init] FATAL: Could not locate or parse " << data_file << std::endl;
        std::cerr << "[Init] Please ensure city_data.json exists in the data directory." << std::endl;
        return 1;
    }

    std::cout << "[Init] Initialized " << graph.getNodeCount() << " locations and "
              << graph.getRoadCount() << " roads in Adjacency List.\n";

    HttpServer server;
    server.set_mount_point("/", frontend_dir);

    EvacuationAPI api(graph);
    api.registerRoutes(server);

    std::cout << "[Init] Mounted frontend dashboard from: " << frontend_dir << std::endl;
    std::cout << "[Init] Starting HTTP backend listener on http://127.0.0.1:" << port << std::endl;
    std::cout << "[Init] Open your browser and navigate to: http://localhost:" << port << std::endl;
    std::cout << "[Init] Press Ctrl+C in this console to terminate the server.\n" << std::endl;

    if (!server.listen("127.0.0.1", port)) {
        std::cerr << "[Init] ERROR: Failed to bind to port " << port << ". Check if another instance is running." << std::endl;
        return 1;
    }

    return 0;
}