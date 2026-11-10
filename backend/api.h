#ifndef API_H
#define API_H

#include "HttpServer.h"
#include "Graph.h"

/**
 * ADSA Project: Intelligent Disaster Evacuation Route Planner
 * Component: REST API Router & Controller
 * 
 * Exposes endpoints for:
 * - Network topology introspection (/api/network, /api/locations, /api/roads)
 * - Algorithm execution (/api/route/shortest, /api/route/safest, /api/bfs)
 * - Dynamic graph state updates (/api/roads/block, /api/roads/reopen, /api/reset)
 */

class EvacuationAPI {
private:
    CityGraph& graph;

    // Helper to resolve location identifiers (ID, Code, or Name) from JSON request
    int resolveNode(const nlohmann::json& body, const std::string& key) const;

public:
    explicit EvacuationAPI(CityGraph& g);

    // Register all routes on the HTTP server instance
    void registerRoutes(HttpServer& server);
};

#endif // API_H