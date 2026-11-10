#include <iostream>
#include <cassert>
#include <cmath>
#include "../backend/Graph.h"
#include "../backend/Algorithms.h"

void printHeader(const std::string& title) {
    std::cout << "\n======================================================\n";
    std::cout << " TEST CASE: " << title << "\n";
    std::cout << "======================================================\n";
}

int main() {
    std::cout << "=== RUNNING ADSA ALGORITHM VALIDATION SUITE ===\n";

    CityGraph graph;
    bool loaded = graph.loadFromJsonFile("data/city_data.json");
    if (!loaded) {
        std::cerr << "FATAL: Could not load data/city_data.json\n";
        return 1;
    }

    int passed = 0;
    int total = 8;

    // ---------------------------------------------------------
    // TEST 1: Shortest Route Calculation (Dijkstra)
    // ---------------------------------------------------------
    printHeader("1. Dijkstra Shortest Route Calculation");
    int hospId = graph.findNodeIndex("City Hospital");
    int shelterAId = graph.findNodeIndex("Emergency Shelter A");
    if (shelterAId == -1) shelterAId = graph.findNodeIndex("SHELTER_A");
    assert(hospId != -1 && shelterAId != -1);

    RouteResult r1 = EvacuationAlgorithms::findShortestRoute(graph, hospId, shelterAId);
    std::cout << "Status: " << (r1.success ? "SUCCESS" : "FAILED") << "\n";
    std::cout << "Source: " << r1.source_name << " -> Dest: " << r1.destination_name << "\n";
    std::cout << "Distance: " << r1.total_distance_km << " km\n";
    std::cout << "Path: ";
    for (size_t i = 0; i < r1.path_node_names.size(); ++i) {
        std::cout << r1.path_node_names[i] << (i + 1 < r1.path_node_names.size() ? " -> " : "\n");
    }
    assert(r1.success == true);
    assert(r1.path_node_ids.front() == hospId);
    assert(r1.path_node_ids.back() == shelterAId);
    assert(r1.total_distance_km > 0.0);
    std::cout << "-> TEST 1 PASSED!\n";
    passed++;

    // ---------------------------------------------------------
    // TEST 2: Safer Alternative Route (Multi-Objective Optimization)
    // ---------------------------------------------------------
    printHeader("2. Safer Alternative Route vs Hazardous Shortcut");
    int parkId = graph.findNodeIndex("Central Park");
    int mktId = graph.findNodeIndex("Market");

    // Shortest path: picks high-risk shortcut R09 (Central Alley, 1.5 km, Risk: High = 6)
    RouteResult rShort = EvacuationAlgorithms::findShortestRoute(graph, parkId, mktId);
    // Safest path: with risk weight 2.0, detours via Shopping Mall (Low Risk roads)
    RouteResult rSafe = EvacuationAlgorithms::findSafestRoute(graph, parkId, mktId, 2.0);

    std::cout << "Shortest Route Path: " << rShort.path_node_names[0] << " -> " << rShort.path_node_names.back()
              << " | Distance: " << rShort.total_distance_km << " km | Risk: " << rShort.total_risk_penalty << "\n";
    std::cout << "Safest Route Path: ";
    for (size_t i = 0; i < rSafe.path_node_names.size(); ++i) {
        std::cout << rSafe.path_node_names[i] << (i + 1 < rSafe.path_node_names.size() ? " -> " : "");
    }
    std::cout << " | Distance: " << rSafe.total_distance_km << " km | Risk: " << rSafe.total_risk_penalty << "\n";

    assert(rShort.success && rSafe.success);
    // Safest route should have strictly lower risk penalty than shortest route
    assert(rSafe.total_risk_penalty < rShort.total_risk_penalty);
    std::cout << "-> TEST 2 PASSED! Safest route smartly avoided high-risk corridor.\n";
    passed++;

    // ---------------------------------------------------------
    // TEST 3: Road Blocking & Dynamic Detour Recalculation
    // ---------------------------------------------------------
    printHeader("3. Road Blocking & Dynamic Recalculation");
    // Block R01 (Hospital Link Way)
    bool blocked = graph.blockRoad("R01");
    assert(blocked);
    std::cout << "Road R01 (Hospital Link Way) is now marked BLOCKED.\n";

    RouteResult rBlocked = EvacuationAlgorithms::findShortestRoute(graph, hospId, shelterAId);
    assert(rBlocked.success);
    // Ensure R01 is NOT in the path
    for (const auto& rid : rBlocked.path_road_ids) {
        assert(rid != "R01");
    }
    std::cout << "New detour distance: " << rBlocked.total_distance_km << " km (Detoured successfully)\n";
    std::cout << "-> TEST 3 PASSED!\n";
    passed++;

    // ---------------------------------------------------------
    // TEST 4: Reopening a Road & Restoring Optimal Path
    // ---------------------------------------------------------
    printHeader("4. Reopening Road & Restoring Route");
    bool reopened = graph.reopenRoad("R01");
    assert(reopened);
    std::cout << "Road R01 reopened.\n";

    RouteResult rReopened = EvacuationAlgorithms::findShortestRoute(graph, hospId, shelterAId);
    assert(rReopened.success);
    assert(std::abs(rReopened.total_distance_km - r1.total_distance_km) < 1e-5);
    std::cout << "Distance restored to original optimal: " << rReopened.total_distance_km << " km\n";
    std::cout << "-> TEST 4 PASSED!\n";
    passed++;

    // ---------------------------------------------------------
    // TEST 5: BFS Traversal & Reachability
    // ---------------------------------------------------------
    printHeader("5. Breadth-First Search (BFS) Traversal");
    BFSResult bfsRes = EvacuationAlgorithms::runBFS(graph, hospId);
    assert(bfsRes.success);
    assert(bfsRes.reachable_count == graph.getNodeCount());
    std::cout << "Total nodes reached from " << bfsRes.source_name << ": " << bfsRes.reachable_count << "\n";
    std::cout << "Traversal Order First 5: ";
    for (size_t i = 0; i < std::min((size_t)5, bfsRes.traversal_order_names.size()); ++i) {
        std::cout << bfsRes.traversal_order_names[i] << " -> ";
    }
    std::cout << "...\n";
    std::cout << "-> TEST 5 PASSED!\n";
    passed++;

    // ---------------------------------------------------------
    // TEST 6: Handling Disconnected / Partitioned Locations
    // ---------------------------------------------------------
    printHeader("6. Handling Disconnected / Unreachable Locations");
    // Shelter B has only two access roads: R13 and R14
    int shelterBId = graph.findNodeIndex("Emergency Shelter B");
    if (shelterBId == -1) shelterBId = graph.findNodeIndex("SHELTER_B");
    assert(shelterBId != -1);
    graph.blockRoad("R13");
    graph.blockRoad("R14");
    std::cout << "Blocked all ingress roads (R13, R14) to Emergency Shelter B.\n";

    RouteResult rDisconnected = EvacuationAlgorithms::findShortestRoute(graph, hospId, shelterBId);
    assert(rDisconnected.success == false);
    std::cout << "Expected Failure Received: " << rDisconnected.message << "\n";
    std::cout << "-> TEST 6 PASSED!\n";
    passed++;

    // ---------------------------------------------------------
    // TEST 7: Handling Invalid Locations Gracefully
    // ---------------------------------------------------------
    printHeader("7. Handling Invalid Location Inputs");
    RouteResult rInvalid = EvacuationAlgorithms::findShortestRoute(graph, -1, 999);
    assert(rInvalid.success == false);
    std::cout << "Error captured: " << rInvalid.message << "\n";
    std::cout << "-> TEST 7 PASSED!\n";
    passed++;

    // ---------------------------------------------------------
    // TEST 8: Resetting City Network to Default State
    // ---------------------------------------------------------
    printHeader("8. Resetting City Network");
    graph.resetNetwork();
    const RoadInfo* r13Info = graph.getRoadById("R13");
    const RoadInfo* r14Info = graph.getRoadById("R14");
    assert(r13Info != nullptr && !r13Info->is_blocked);
    assert(r14Info != nullptr && !r14Info->is_blocked);

    RouteResult rAfterReset = EvacuationAlgorithms::findShortestRoute(graph, hospId, shelterBId);
    assert(rAfterReset.success == true);
    std::cout << "Shelter B is reachable again after reset! Distance: " << rAfterReset.total_distance_km << " km\n";
    std::cout << "-> TEST 8 PASSED!\n";
    passed++;

    std::cout << "\n======================================================\n";
    std::cout << " ALL " << passed << " / " << total << " ADSA ALGORITHM TESTS PASSED PERFECTLY!\n";
    std::cout << "======================================================\n";

    return 0;
}