#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "Graph.h"
#include "include/json.hpp"
#include <vector>
#include <string>

/**
 * ADSA Project: Intelligent Disaster Evacuation Route Planner
 * Component: Graph Algorithms & Optimization Engine
 * 
 * Implements:
 * 1. Dijkstra's Algorithm (Shortest Physical Distance)
 * 2. Multi-Objective Safety Optimization (Risk-Weighted Dijkstra)
 * 3. Breadth-First Search (BFS Evacuation Reachability & Traversal)
 */

struct RouteStep {
    int from_id;
    std::string from_name;
    int to_id;
    std::string to_name;
    std::string road_id;
    std::string road_name;
    double segment_distance;
    std::string risk_level;
    int risk_penalty;
};

struct RouteResult {
    bool success;
    std::string algorithm_name;
    std::string message;
    int source_id;
    std::string source_name;
    int destination_id;
    std::string destination_name;

    std::vector<int> path_node_ids;
    std::vector<std::string> path_node_names;
    std::vector<std::string> path_road_ids;
    std::vector<RouteStep> steps;

    double total_distance_km;
    int total_risk_penalty;
    double composite_cost;
    double risk_weight_used;
    double estimated_time_minutes;

    int nodes_visited_count;
    std::vector<int> exploration_order;

    std::string safety_grade;
    std::string complexity_time;
    std::string complexity_space;
    std::string algorithm_explanation;

    nlohmann::json toJson() const;
};

struct BFSResult {
    bool success;
    std::string message;
    int source_id;
    std::string source_name;
    std::vector<int> traversal_order_ids;
    std::vector<std::string> traversal_order_names;
    int reachable_count;
    int unreachable_count;
    std::vector<std::string> unreachable_names;
    
    std::string complexity_time;
    std::string complexity_space;
    std::string algorithm_explanation;

    nlohmann::json toJson() const;
};

class EvacuationAlgorithms {
public:
    // Core Dijkstra Algorithm for shortest physical distance (ignoring blocked edges)
    static RouteResult findShortestRoute(const CityGraph& graph, int source_id, int destination_id);

    // Multi-objective Safety Optimization using Risk-Weighted Dijkstra
    // Cost = Distance + (Risk Penalty * Risk Weight)
    static RouteResult findSafestRoute(const CityGraph& graph, int source_id, int destination_id, double risk_weight = 2.0);

    // Breadth-First Search (BFS) for Reachable Locations Analysis
    static BFSResult runBFS(const CityGraph& graph, int source_id);

private:
    static std::string calculateSafetyGrade(int total_risk, size_t hops);
};

#endif // ALGORITHMS_H