#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include "include/json.hpp"

/**
 * ADSA Project: Intelligent Disaster Evacuation Route Planner
 * Component: Graph Representation (Adjacency List)
 * 
 * Vertices: City Locations (Hospitals, Shelters, Schools, Stations, etc.)
 * Edges: City Roads (Bidirectional, Weighted by Distance and Safety Risk)
 */

// Structure representing an outgoing edge in the Adjacency List
struct Edge {
    int to;                     // Destination vertex ID
    std::string road_id;        // Unique road identifier (e.g. "R01")
    std::string road_name;      // Human-readable road name
    double distance;            // Physical road length in kilometres
    std::string risk_level;     // "Low", "Medium", "High"
    int risk_penalty;           // Quantitative hazard penalty (1, 3, 6)
    bool is_blocked;            // Dynamic disaster blockage status
};

// Structure representing a city location (Vertex)
struct LocationNode {
    int id;                     // 0-indexed vertex identifier
    std::string code;           // System code (e.g. "LOC_HOSP", "SHELTER_A")
    std::string name;           // Display name (e.g. "City Hospital")
    std::string type;           // "hospital", "shelter", "police", etc.
    double x;                   // Visual SVG Cartesian X coordinate
    double y;                   // Visual SVG Cartesian Y coordinate
    bool is_shelter;            // Evacuation destination flag
    int capacity;               // Human shelter or facility capacity
    std::string description;    // Functional emergency notes
};

// Master road record for listing, toggling, and network persistence
struct RoadInfo {
    std::string id;             // Road ID
    std::string name;           // Road Name
    int u;                      // Vertex 1 ID
    int v;                      // Vertex 2 ID
    double distance_km;         // Distance in km
    std::string risk_level;     // Risk string
    int risk_penalty;           // Numeric penalty
    bool is_blocked;            // Current blockage state
};

class CityGraph {
private:
    std::string city_name;
    double average_speed_kmh;
    std::unordered_map<std::string, int> risk_weights_map;

    std::vector<LocationNode> nodes;
    std::vector<RoadInfo> roads;
    std::vector<RoadInfo> original_roads; // Snapshot for reset functionality

    // Core ADSA Adjacency List: adj[u] contains all bidirectional incident edges
    std::vector<std::vector<Edge>> adj;

    // Fast O(1) lookup tables
    std::unordered_map<std::string, int> name_to_id;
    std::unordered_map<std::string, int> code_to_id;
    std::unordered_map<std::string, size_t> road_id_to_idx;

    void rebuildAdjacencyList();

public:
    CityGraph();

    // Data ingestion
    bool loadFromJsonFile(const std::string& filepath);
    bool loadFromJson(const nlohmann::json& j);

    // Dynamic Road Management
    bool blockRoad(const std::string& road_id);
    bool reopenRoad(const std::string& road_id);
    void resetNetwork();

    // Lookups & Accessors
    int getNodeCount() const;
    int getRoadCount() const;
    int findNodeIndex(const std::string& identifier) const;
    const LocationNode& getNode(int id) const;
    const std::vector<LocationNode>& getAllNodes() const;
    const std::vector<RoadInfo>& getAllRoads() const;
    const std::vector<Edge>& getEdges(int u) const;
    const RoadInfo* getRoadById(const std::string& road_id) const;

    std::string getCityName() const { return city_name; }
    double getAverageSpeedKmh() const { return average_speed_kmh; }
    int getRiskPenalty(const std::string& level) const;

    // Serialization helper for frontend API
    nlohmann::json toJson() const;
};

#endif // GRAPH_H