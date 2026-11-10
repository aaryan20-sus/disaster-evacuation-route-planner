#include "Algorithms.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <sstream>

const double INF_DIST = std::numeric_limits<double>::infinity();

std::string EvacuationAlgorithms::calculateSafetyGrade(int total_risk, size_t hops) {
    if (hops == 0) return "Safe";
    double avgRisk = static_cast<double>(total_risk) / hops;
    if (avgRisk <= 1.5) {
        return "High Safety (Low Hazard Exposure)";
    } else if (avgRisk <= 3.2) {
        return "Moderate Safety (Acceptable Risk Corridor)";
    } else {
        return "Elevated Hazard (Active Caution Advisories)";
    }
}

RouteResult EvacuationAlgorithms::findShortestRoute(const CityGraph& graph, int source_id, int destination_id) {
    RouteResult result;
    result.algorithm_name = "Dijkstra's Shortest Path Algorithm";
    result.complexity_time = "O((V + E) log V)";
    result.complexity_space = "O(V + E)";
    result.risk_weight_used = 0.0;
    result.success = false;

    int n = graph.getNodeCount();
    if (source_id < 0 || source_id >= n || destination_id < 0 || destination_id >= n) {
        result.message = "Error: Invalid source or destination location ID provided.";
        return result;
    }

    result.source_id = source_id;
    result.source_name = graph.getNode(source_id).name;
    result.destination_id = destination_id;
    result.destination_name = graph.getNode(destination_id).name;

    if (source_id == destination_id) {
        result.success = true;
        result.message = "Origin and destination are the exact same location.";
        result.total_distance_km = 0.0;
        result.total_risk_penalty = 0;
        result.composite_cost = 0.0;
        result.estimated_time_minutes = 0.0;
        result.nodes_visited_count = 1;
        result.path_node_ids.push_back(source_id);
        result.path_node_names.push_back(result.source_name);
        result.safety_grade = "Safe";
        result.algorithm_explanation = "Source matches destination; no traversal required.";
        return result;
    }

    // Min-heap Priority Queue storing pairs of: (distance, vertex_id)
    std::priority_queue<std::pair<double, int>, 
                        std::vector<std::pair<double, int>>, 
                        std::greater<std::pair<double, int>>> pq;

    std::vector<double> dist(n, INF_DIST);
    std::vector<int> parent(n, -1);
    std::vector<Edge> parent_edge(n);
    std::vector<bool> visited(n, false);

    dist[source_id] = 0.0;
    pq.push({0.0, source_id});

    while (!pq.empty()) {
        auto top = pq.top();
        pq.pop();
        double current_d = top.first;
        int u = top.second;

        if (visited[u]) continue;
        visited[u] = true;
        result.exploration_order.push_back(u);

        // Early exit optimization once destination is settled
        if (u == destination_id) {
            break;
        }

        if (current_d > dist[u]) continue;

        for (const auto& edge : graph.getEdges(u)) {
            // Strict ADSA requirement: Ignore road blockages caused by disaster
            if (edge.is_blocked) {
                continue;
            }

            int v = edge.to;
            double edge_cost = edge.distance; // Pure shortest distance metric

            if (dist[u] + edge_cost < dist[v]) {
                dist[v] = dist[u] + edge_cost;
                parent[v] = u;
                parent_edge[v] = edge;
                pq.push({dist[v], v});
            }
        }
    }

    result.nodes_visited_count = (int)result.exploration_order.size();

    // Check if destination was reached
    if (dist[destination_id] == INF_DIST) {
        result.success = false;
        result.message = "No available evacuation route found. All connecting paths are currently blocked or partitioned by disaster conditions.";
        result.algorithm_explanation = "Dijkstra exhausted all accessible edges without reaching the emergency shelter due to active road blockages.";
        return result;
    }

    // Path Reconstruction by backtracking through parent pointers
    std::vector<int> path_nodes;
    std::vector<Edge> path_edges;
    int curr = destination_id;

    while (curr != -1) {
        path_nodes.push_back(curr);
        if (parent[curr] != -1) {
            path_edges.push_back(parent_edge[curr]);
        }
        curr = parent[curr];
    }

    std::reverse(path_nodes.begin(), path_nodes.end());
    std::reverse(path_edges.begin(), path_edges.end());

    result.path_node_ids = path_nodes;
    for (int nid : path_nodes) {
        result.path_node_names.push_back(graph.getNode(nid).name);
    }

    double total_distance = 0.0;
    int total_risk = 0;

    for (const auto& e : path_edges) {
        result.path_road_ids.push_back(e.road_id);
        total_distance += e.distance;
        total_risk += e.risk_penalty;

        RouteStep step;
        step.from_id = (curr == -1 ? 0 : 0); // populated properly below
        step.to_id = e.to;
        step.to_name = graph.getNode(e.to).name;
        step.road_id = e.road_id;
        step.road_name = e.road_name;
        step.segment_distance = e.distance;
        step.risk_level = e.risk_level;
        step.risk_penalty = e.risk_penalty;
        result.steps.push_back(step);
    }

    // Fix steps from_id / from_name correctly
    for (size_t i = 0; i < result.steps.size(); ++i) {
        result.steps[i].from_id = path_nodes[i];
        result.steps[i].from_name = graph.getNode(path_nodes[i]).name;
    }

    result.success = true;
    result.total_distance_km = total_distance;
    result.total_risk_penalty = total_risk;
    result.composite_cost = total_distance;
    result.estimated_time_minutes = (total_distance / graph.getAverageSpeedKmh()) * 60.0;
    result.safety_grade = calculateSafetyGrade(total_risk, path_edges.size());
    result.message = "Shortest evacuation route calculated successfully.";

    std::ostringstream ss;
    ss << "Dijkstra evaluated shortest geometric distance (" << std::fixed << std::setprecision(1)
       << total_distance << " km across " << path_edges.size() << " road segments, traversing "
       << result.nodes_visited_count << " nodes).";
    result.algorithm_explanation = ss.str();

    return result;
}

RouteResult EvacuationAlgorithms::findSafestRoute(const CityGraph& graph, int source_id, int destination_id, double risk_weight) {
    RouteResult result;
    result.algorithm_name = "Safety-Optimized Multi-Objective Routing";
    result.complexity_time = "O((V + E) log V)";
    result.complexity_space = "O(V + E)";
    result.risk_weight_used = risk_weight;
    result.success = false;

    int n = graph.getNodeCount();
    if (source_id < 0 || source_id >= n || destination_id < 0 || destination_id >= n) {
        result.message = "Error: Invalid source or destination location ID provided.";
        return result;
    }

    result.source_id = source_id;
    result.source_name = graph.getNode(source_id).name;
    result.destination_id = destination_id;
    result.destination_name = graph.getNode(destination_id).name;

    if (source_id == destination_id) {
        result.success = true;
        result.message = "Origin and destination are the exact same location.";
        result.total_distance_km = 0.0;
        result.total_risk_penalty = 0;
        result.composite_cost = 0.0;
        result.estimated_time_minutes = 0.0;
        result.nodes_visited_count = 1;
        result.path_node_ids.push_back(source_id);
        result.path_node_names.push_back(result.source_name);
        result.safety_grade = "Safe";
        result.algorithm_explanation = "Source matches destination; no traversal required.";
        return result;
    }

    // Priority Queue storing (composite_cost, vertex_id)
    std::priority_queue<std::pair<double, int>, 
                        std::vector<std::pair<double, int>>, 
                        std::greater<std::pair<double, int>>> pq;

    std::vector<double> min_cost(n, INF_DIST);
    std::vector<int> parent(n, -1);
    std::vector<Edge> parent_edge(n);
    std::vector<bool> visited(n, false);

    min_cost[source_id] = 0.0;
    pq.push({0.0, source_id});

    while (!pq.empty()) {
        auto top = pq.top();
        pq.pop();
        double current_c = top.first;
        int u = top.second;

        if (visited[u]) continue;
        visited[u] = true;
        result.exploration_order.push_back(u);

        if (u == destination_id) {
            break;
        }

        if (current_c > min_cost[u]) continue;

        for (const auto& edge : graph.getEdges(u)) {
            // Disaster Blockage check
            if (edge.is_blocked) {
                continue;
            }

            int v = edge.to;
            // Core ADSA Formula: Route Cost = Distance + (Risk Penalty * Risk Weight)
            double edge_composite = edge.distance + (static_cast<double>(edge.risk_penalty) * risk_weight);

            if (min_cost[u] + edge_composite < min_cost[v]) {
                min_cost[v] = min_cost[u] + edge_composite;
                parent[v] = u;
                parent_edge[v] = edge;
                pq.push({min_cost[v], v});
            }
        }
    }

    result.nodes_visited_count = (int)result.exploration_order.size();

    if (min_cost[destination_id] == INF_DIST) {
        result.success = false;
        result.message = "No viable safe evacuation corridor found. Target shelter is unreachable due to severe road closures.";
        result.algorithm_explanation = "Safety routing could not identify an open continuous path to the shelter.";
        return result;
    }

    // Reconstruct Path
    std::vector<int> path_nodes;
    std::vector<Edge> path_edges;
    int curr = destination_id;

    while (curr != -1) {
        path_nodes.push_back(curr);
        if (parent[curr] != -1) {
            path_edges.push_back(parent_edge[curr]);
        }
        curr = parent[curr];
    }

    std::reverse(path_nodes.begin(), path_nodes.end());
    std::reverse(path_edges.begin(), path_edges.end());

    result.path_node_ids = path_nodes;
    for (int nid : path_nodes) {
        result.path_node_names.push_back(graph.getNode(nid).name);
    }

    // Critical Requirement: Separate physical distance and accumulated risk penalty
    double total_distance = 0.0;
    int total_risk = 0;

    for (const auto& e : path_edges) {
        result.path_road_ids.push_back(e.road_id);
        total_distance += e.distance;
        total_risk += e.risk_penalty;

        RouteStep step;
        step.from_id = 0;
        step.to_id = e.to;
        step.to_name = graph.getNode(e.to).name;
        step.road_id = e.road_id;
        step.road_name = e.road_name;
        step.segment_distance = e.distance;
        step.risk_level = e.risk_level;
        step.risk_penalty = e.risk_penalty;
        result.steps.push_back(step);
    }

    for (size_t i = 0; i < result.steps.size(); ++i) {
        result.steps[i].from_id = path_nodes[i];
        result.steps[i].from_name = graph.getNode(path_nodes[i]).name;
    }

    result.success = true;
    result.total_distance_km = total_distance;
    result.total_risk_penalty = total_risk;
    result.composite_cost = min_cost[destination_id];
    result.estimated_time_minutes = (total_distance / graph.getAverageSpeedKmh()) * 60.0;
    result.safety_grade = calculateSafetyGrade(total_risk, path_edges.size());
    result.message = "Safest evacuation route calculated successfully with risk balancing.";

    std::ostringstream ss;
    ss << "Safety optimization applied a risk penalty multiplier of " << std::fixed << std::setprecision(1)
       << risk_weight << " to steer away from hazardous sectors. Physical distance: "
       << total_distance << " km, Cumulative Hazard Risk: " << total_risk
       << ", Composite Metric: " << result.composite_cost << ".";
    result.algorithm_explanation = ss.str();

    return result;
}

BFSResult EvacuationAlgorithms::runBFS(const CityGraph& graph, int source_id) {
    BFSResult result;
    result.complexity_time = "O(V + E)";
    result.complexity_space = "O(V)";
    result.success = false;

    int n = graph.getNodeCount();
    if (source_id < 0 || source_id >= n) {
        result.message = "Error: Invalid source ID provided for BFS.";
        return result;
    }

    result.source_id = source_id;
    result.source_name = graph.getNode(source_id).name;

    std::vector<bool> visited(n, false);
    std::queue<int> q;

    visited[source_id] = true;
    q.push(source_id);

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        result.traversal_order_ids.push_back(u);
        result.traversal_order_names.push_back(graph.getNode(u).name);

        for (const auto& edge : graph.getEdges(u)) {
            // BFS respects road closures
            if (edge.is_blocked) continue;

            int v = edge.to;
            if (!visited[v]) {
                visited[v] = true;
                q.push(v);
            }
        }
    }

    result.reachable_count = (int)result.traversal_order_ids.size();
    result.unreachable_count = 0;

    for (int i = 0; i < n; ++i) {
        if (!visited[i]) {
            result.unreachable_count++;
            result.unreachable_names.push_back(graph.getNode(i).name);
        }
    }

    result.success = true;
    result.message = "BFS accessibility exploration completed.";

    std::ostringstream ss;
    ss << "BFS explored level-by-level starting from " << result.source_name
       << ". Reached " << result.reachable_count << " out of " << n
       << " city sectors using currently unblocked roadways.";
    result.algorithm_explanation = ss.str();

    return result;
}

nlohmann::json RouteResult::toJson() const {
    nlohmann::json j;
    j["success"] = success;
    j["algorithm_name"] = algorithm_name;
    j["message"] = message;
    j["source_id"] = source_id;
    j["source_name"] = source_name;
    j["destination_id"] = destination_id;
    j["destination_name"] = destination_name;
    j["total_distance_km"] = total_distance_km;
    j["total_risk_penalty"] = total_risk_penalty;
    j["composite_cost"] = composite_cost;
    j["risk_weight_used"] = risk_weight_used;
    j["estimated_time_minutes"] = estimated_time_minutes;
    j["nodes_visited_count"] = nodes_visited_count;
    j["safety_grade"] = safety_grade;
    j["complexity_time"] = complexity_time;
    j["complexity_space"] = complexity_space;
    j["algorithm_explanation"] = algorithm_explanation;

    j["path_node_ids"] = path_node_ids;
    j["path_node_names"] = path_node_names;
    j["path_road_ids"] = path_road_ids;
    j["exploration_order"] = exploration_order;

    nlohmann::json stepsArray = nlohmann::json::array();
    for (const auto& s : steps) {
        stepsArray.push_back({
            {"from_id", s.from_id},
            {"from_name", s.from_name},
            {"to_id", s.to_id},
            {"to_name", s.to_name},
            {"road_id", s.road_id},
            {"road_name", s.road_name},
            {"segment_distance", s.segment_distance},
            {"risk_level", s.risk_level},
            {"risk_penalty", s.risk_penalty}
        });
    }
    j["steps"] = stepsArray;
    return j;
}

nlohmann::json BFSResult::toJson() const {
    nlohmann::json j;
    j["success"] = success;
    j["message"] = message;
    j["source_id"] = source_id;
    j["source_name"] = source_name;
    j["reachable_count"] = reachable_count;
    j["unreachable_count"] = unreachable_count;
    j["traversal_order_ids"] = traversal_order_ids;
    j["traversal_order_names"] = traversal_order_names;
    j["unreachable_names"] = unreachable_names;
    j["complexity_time"] = complexity_time;
    j["complexity_space"] = complexity_space;
    j["algorithm_explanation"] = algorithm_explanation;
    return j;
}