#include "api.h"
#include "Algorithms.h"
#include <iostream>

EvacuationAPI::EvacuationAPI(CityGraph& g) : graph(g) {}

int EvacuationAPI::resolveNode(const nlohmann::json& body, const std::string& key) const {
    if (!body.contains(key)) return -1;
    if (body[key].is_number_integer()) {
        int id = body[key].get<int>();
        if (id >= 0 && id < graph.getNodeCount()) return id;
    } else if (body[key].is_string()) {
        std::string str = body[key].get<std::string>();
        return graph.findNodeIndex(str);
    }
    return -1;
}

void EvacuationAPI::registerRoutes(HttpServer& server) {
    // -------------------------------------------------------------
    // GET /api/network - Complete city graph data
    // -------------------------------------------------------------
    server.Get("/api/network", [this](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        nlohmann::json j = graph.toJson();
        res.set_status(200, "OK");
        res.set_content(j.dump(2), "application/json");
    });

    // -------------------------------------------------------------
    // GET /api/locations - Array of city locations
    // -------------------------------------------------------------
    server.Get("/api/locations", [this](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& node : graph.getAllNodes()) {
            arr.push_back({
                {"id", node.id},
                {"code", node.code},
                {"name", node.name},
                {"type", node.type},
                {"x", node.x},
                {"y", node.y},
                {"is_shelter", node.is_shelter},
                {"capacity", node.capacity},
                {"description", node.description}
            });
        }
        nlohmann::json response;
        response["locations"] = arr;
        res.set_status(200, "OK");
        res.set_content(response.dump(2), "application/json");
    });

    // -------------------------------------------------------------
    // GET /api/roads - Array of road segments with current blockage status
    // -------------------------------------------------------------
    server.Get("/api/roads", [this](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& r : graph.getAllRoads()) {
            arr.push_back({
                {"id", r.id},
                {"name", r.name},
                {"u", r.u},
                {"u_name", graph.getNode(r.u).name},
                {"v", r.v},
                {"v_name", graph.getNode(r.v).name},
                {"distance_km", r.distance_km},
                {"risk_level", r.risk_level},
                {"risk_penalty", r.risk_penalty},
                {"is_blocked", r.is_blocked}
            });
        }
        nlohmann::json response;
        response["roads"] = arr;
        res.set_status(200, "OK");
        res.set_content(response.dump(2), "application/json");
    });

    // -------------------------------------------------------------
    // POST /api/route/shortest - Dijkstra shortest distance
    // -------------------------------------------------------------
    server.Post("/api/route/shortest", [this](const HttpRequest& req, HttpResponse& res) {
        try {
            nlohmann::json body = nlohmann::json::parse(req.body);
            int src = resolveNode(body, "source");
            int dst = resolveNode(body, "destination");

            if (src == -1 || dst == -1) {
                nlohmann::json err;
                err["success"] = false;
                err["error"] = "Invalid source or destination location identifier.";
                res.set_status(400, "Bad Request");
                res.set_content(err.dump(2), "application/json");
                return;
            }

            RouteResult result = EvacuationAlgorithms::findShortestRoute(graph, src, dst);
            res.set_status(200, "OK");
            res.set_content(result.toJson().dump(2), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["success"] = false;
            err["error"] = std::string("JSON Request parsing failed: ") + e.what();
            res.set_status(400, "Bad Request");
            res.set_content(err.dump(2), "application/json");
        }
    });

    // -------------------------------------------------------------
    // POST /api/route/safest - Multi-Objective Risk Weighted Routing
    // -------------------------------------------------------------
    server.Post("/api/route/safest", [this](const HttpRequest& req, HttpResponse& res) {
        try {
            nlohmann::json body = nlohmann::json::parse(req.body);
            int src = resolveNode(body, "source");
            int dst = resolveNode(body, "destination");
            double risk_weight = body.value("risk_weight", 2.0);

            if (src == -1 || dst == -1) {
                nlohmann::json err;
                err["success"] = false;
                err["error"] = "Invalid source or destination location identifier.";
                res.set_status(400, "Bad Request");
                res.set_content(err.dump(2), "application/json");
                return;
            }

            RouteResult result = EvacuationAlgorithms::findSafestRoute(graph, src, dst, risk_weight);
            res.set_status(200, "OK");
            res.set_content(result.toJson().dump(2), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["success"] = false;
            err["error"] = std::string("JSON Request parsing failed: ") + e.what();
            res.set_status(400, "Bad Request");
            res.set_content(err.dump(2), "application/json");
        }
    });

    // -------------------------------------------------------------
    // POST /api/bfs - Breadth-First Search reachability exploration
    // -------------------------------------------------------------
    server.Post("/api/bfs", [this](const HttpRequest& req, HttpResponse& res) {
        try {
            nlohmann::json body = nlohmann::json::parse(req.body);
            int src = resolveNode(body, "source");

            if (src == -1) {
                nlohmann::json err;
                err["success"] = false;
                err["error"] = "Invalid source location identifier.";
                res.set_status(400, "Bad Request");
                res.set_content(err.dump(2), "application/json");
                return;
            }

            BFSResult result = EvacuationAlgorithms::runBFS(graph, src);
            res.set_status(200, "OK");
            res.set_content(result.toJson().dump(2), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["success"] = false;
            err["error"] = std::string("JSON Request parsing failed: ") + e.what();
            res.set_status(400, "Bad Request");
            res.set_content(err.dump(2), "application/json");
        }
    });

    // -------------------------------------------------------------
    // POST /api/roads/block - Dynamically block a road
    // -------------------------------------------------------------
    server.Post("/api/roads/block", [this](const HttpRequest& req, HttpResponse& res) {
        try {
            nlohmann::json body = nlohmann::json::parse(req.body);
            std::string road_id = body.value("road_id", "");

            if (road_id.empty() || !graph.blockRoad(road_id)) {
                nlohmann::json err;
                err["success"] = false;
                err["error"] = "Road ID not found: " + road_id;
                res.set_status(404, "Not Found");
                res.set_content(err.dump(2), "application/json");
                return;
            }

            const RoadInfo* r = graph.getRoadById(road_id);
            nlohmann::json resp;
            resp["success"] = true;
            resp["message"] = "Road " + road_id + " (" + (r ? r->name : "") + ") is now BLOCKED.";
            resp["road_id"] = road_id;
            resp["is_blocked"] = true;

            res.set_status(200, "OK");
            res.set_content(resp.dump(2), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["success"] = false;
            err["error"] = std::string("JSON Request parsing failed: ") + e.what();
            res.set_status(400, "Bad Request");
            res.set_content(err.dump(2), "application/json");
        }
    });

    // -------------------------------------------------------------
    // POST /api/roads/reopen - Dynamically reopen a road
    // -------------------------------------------------------------
    server.Post("/api/roads/reopen", [this](const HttpRequest& req, HttpResponse& res) {
        try {
            nlohmann::json body = nlohmann::json::parse(req.body);
            std::string road_id = body.value("road_id", "");

            if (road_id.empty() || !graph.reopenRoad(road_id)) {
                nlohmann::json err;
                err["success"] = false;
                err["error"] = "Road ID not found: " + road_id;
                res.set_status(404, "Not Found");
                res.set_content(err.dump(2), "application/json");
                return;
            }

            const RoadInfo* r = graph.getRoadById(road_id);
            nlohmann::json resp;
            resp["success"] = true;
            resp["message"] = "Road " + road_id + " (" + (r ? r->name : "") + ") has been REOPENED.";
            resp["road_id"] = road_id;
            resp["is_blocked"] = false;

            res.set_status(200, "OK");
            res.set_content(resp.dump(2), "application/json");
        } catch (const std::exception& e) {
            nlohmann::json err;
            err["success"] = false;
            err["error"] = std::string("JSON Request parsing failed: ") + e.what();
            res.set_status(400, "Bad Request");
            res.set_content(err.dump(2), "application/json");
        }
    });

    // -------------------------------------------------------------
    // POST /api/reset - Restore initial city network state
    // -------------------------------------------------------------
    server.Post("/api/reset", [this](const HttpRequest& req, HttpResponse& res) {
        (void)req;
        graph.resetNetwork();
        nlohmann::json resp;
        resp["success"] = true;
        resp["message"] = "City road network has been completely restored to baseline operational state.";
        res.set_status(200, "OK");
        res.set_content(resp.dump(2), "application/json");
    });
}