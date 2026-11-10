#include "Graph.h"
#include <fstream>
#include <iostream>
#include <sstream>

CityGraph::CityGraph() : city_name("Metropolis SafeCity"), average_speed_kmh(30.0) {
    risk_weights_map["Low"] = 1;
    risk_weights_map["Medium"] = 3;
    risk_weights_map["High"] = 6;
}

int CityGraph::getRiskPenalty(const std::string& level) const {
    auto it = risk_weights_map.find(level);
    if (it != risk_weights_map.end()) {
        return it->second;
    }
    return 1; // Default low risk penalty
}

bool CityGraph::loadFromJsonFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[CityGraph] Error: Failed to open city data file: " << filepath << std::endl;
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;
        return loadFromJson(j);
    } catch (const std::exception& e) {
        std::cerr << "[CityGraph] JSON parsing exception: " << e.what() << std::endl;
        return false;
    }
}

bool CityGraph::loadFromJson(const nlohmann::json& j) {
    try {
        nodes.clear();
        roads.clear();
        original_roads.clear();
        name_to_id.clear();
        code_to_id.clear();
        road_id_to_idx.clear();

        if (j.contains("city_name") && j["city_name"].is_string()) {
            city_name = j["city_name"].get<std::string>();
        }
        if (j.contains("average_speed_kmh") && j["average_speed_kmh"].is_number()) {
            average_speed_kmh = j["average_speed_kmh"].get<double>();
        }

        if (j.contains("risk_penalties") && j["risk_penalties"].is_object()) {
            for (auto& el : j["risk_penalties"].items()) {
                risk_weights_map[el.key()] = el.value().get<int>();
            }
        }

        // Parse Vertices (Locations)
        if (j.contains("locations") && j["locations"].is_array()) {
            for (const auto& loc : j["locations"]) {
                LocationNode node;
                node.id = loc.value("id", (int)nodes.size());
                node.code = loc.value("code", "LOC_" + std::to_string(node.id));
                node.name = loc.value("name", "Unknown Location");
                node.type = loc.value("type", "general");
                node.x = loc.value("x", 0.0);
                node.y = loc.value("y", 0.0);
                node.is_shelter = loc.value("is_shelter", false);
                node.capacity = loc.value("capacity", 100);
                node.description = loc.value("description", "");

                name_to_id[node.name] = node.id;
                code_to_id[node.code] = node.id;
                nodes.push_back(node);
            }
        }

        // Parse Edges (Roads)
        if (j.contains("roads") && j["roads"].is_array()) {
            for (const auto& r : j["roads"]) {
                RoadInfo road;
                road.id = r.value("id", "R" + std::to_string(roads.size() + 1));
                road.name = r.value("name", "Road " + road.id);
                road.u = r.value("u", 0);
                road.v = r.value("v", 0);
                road.distance_km = r.value("distance_km", 1.0);
                road.risk_level = r.value("risk_level", "Low");
                road.risk_penalty = getRiskPenalty(road.risk_level);
                road.is_blocked = r.value("is_blocked", false);

                road_id_to_idx[road.id] = roads.size();
                roads.push_back(road);
            }
        }

        original_roads = roads; // Store immutable backup for reset
        rebuildAdjacencyList();

        std::cout << "[CityGraph] Successfully loaded " << nodes.size() 
                  << " locations and " << roads.size() << " roads." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[CityGraph] Load exception: " << e.what() << std::endl;
        return false;
    }
}

void CityGraph::rebuildAdjacencyList() {
    adj.assign(nodes.size(), std::vector<Edge>());

    for (const auto& r : roads) {
        if (r.u >= 0 && r.u < (int)nodes.size() && r.v >= 0 && r.v < (int)nodes.size()) {
            // Forward Edge: u -> v
            Edge forwardEdge;
            forwardEdge.to = r.v;
            forwardEdge.road_id = r.id;
            forwardEdge.road_name = r.name;
            forwardEdge.distance = r.distance_km;
            forwardEdge.risk_level = r.risk_level;
            forwardEdge.risk_penalty = r.risk_penalty;
            forwardEdge.is_blocked = r.is_blocked;
            adj[r.u].push_back(forwardEdge);

            // Reverse Edge: v -> u (Bidirectional city roads)
            Edge reverseEdge;
            reverseEdge.to = r.u;
            reverseEdge.road_id = r.id;
            reverseEdge.road_name = r.name;
            reverseEdge.distance = r.distance_km;
            reverseEdge.risk_level = r.risk_level;
            reverseEdge.risk_penalty = r.risk_penalty;
            reverseEdge.is_blocked = r.is_blocked;
            adj[r.v].push_back(reverseEdge);
        }
    }
}

bool CityGraph::blockRoad(const std::string& road_id) {
    auto it = road_id_to_idx.find(road_id);
    if (it == road_id_to_idx.end()) {
        return false;
    }

    size_t idx = it->second;
    roads[idx].is_blocked = true;

    // Update Adjacency List for both endpoints
    int u = roads[idx].u;
    int v = roads[idx].v;

    for (auto& edge : adj[u]) {
        if (edge.road_id == road_id) {
            edge.is_blocked = true;
        }
    }
    for (auto& edge : adj[v]) {
        if (edge.road_id == road_id) {
            edge.is_blocked = true;
        }
    }
    return true;
}

bool CityGraph::reopenRoad(const std::string& road_id) {
    auto it = road_id_to_idx.find(road_id);
    if (it == road_id_to_idx.end()) {
        return false;
    }

    size_t idx = it->second;
    roads[idx].is_blocked = false;

    // Update Adjacency List for both endpoints
    int u = roads[idx].u;
    int v = roads[idx].v;

    for (auto& edge : adj[u]) {
        if (edge.road_id == road_id) {
            edge.is_blocked = false;
        }
    }
    for (auto& edge : adj[v]) {
        if (edge.road_id == road_id) {
            edge.is_blocked = false;
        }
    }
    return true;
}

void CityGraph::resetNetwork() {
    roads = original_roads;
    rebuildAdjacencyList();
}

int CityGraph::getNodeCount() const {
    return (int)nodes.size();
}

int CityGraph::getRoadCount() const {
    return (int)roads.size();
}

int CityGraph::findNodeIndex(const std::string& identifier) const {
    // 1. Try exact name match
    auto itName = name_to_id.find(identifier);
    if (itName != name_to_id.end()) {
        return itName->second;
    }

    // 2. Try exact code match
    auto itCode = code_to_id.find(identifier);
    if (itCode != code_to_id.end()) {
        return itCode->second;
    }

    // 3. Try integer ID parsing
    try {
        size_t pos = 0;
        int id = std::stoi(identifier, &pos);
        if (pos == identifier.length() && id >= 0 && id < (int)nodes.size()) {
            return id;
        }
    } catch (...) {}

    return -1; // Not found
}

const LocationNode& CityGraph::getNode(int id) const {
    return nodes.at(id);
}

const std::vector<LocationNode>& CityGraph::getAllNodes() const {
    return nodes;
}

const std::vector<RoadInfo>& CityGraph::getAllRoads() const {
    return roads;
}

const std::vector<Edge>& CityGraph::getEdges(int u) const {
    return adj.at(u);
}

const RoadInfo* CityGraph::getRoadById(const std::string& road_id) const {
    auto it = road_id_to_idx.find(road_id);
    if (it != road_id_to_idx.end()) {
        return &roads[it->second];
    }
    return nullptr;
}

nlohmann::json CityGraph::toJson() const {
    nlohmann::json j;
    j["city_name"] = city_name;
    j["average_speed_kmh"] = average_speed_kmh;
    j["total_locations"] = nodes.size();
    j["total_roads"] = roads.size();

    nlohmann::json locArray = nlohmann::json::array();
    for (const auto& node : nodes) {
        locArray.push_back({
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
    j["locations"] = locArray;

    nlohmann::json roadArray = nlohmann::json::array();
    for (const auto& r : roads) {
        roadArray.push_back({
            {"id", r.id},
            {"name", r.name},
            {"u", r.u},
            {"v", r.v},
            {"distance_km", r.distance_km},
            {"risk_level", r.risk_level},
            {"risk_penalty", r.risk_penalty},
            {"is_blocked", r.is_blocked}
        });
    }
    j["roads"] = roadArray;
    return j;
}