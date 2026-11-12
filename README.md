# Intelligent Disaster Evacuation Route Planner
### Advanced Data Structures & Algorithms (ADSA) Project | B.Tech CSE (3rd Semester)

[![C++14](https://img.shields.io/badge/C%2B%2B-14-blue.svg?style=flat-square&logo=c%2B%2B)](https://isocpp.org/)
[![MinGW GCC](https://img.shields.io/badge/Compiler-MinGW%20GCC-green.svg?style=flat-square)](https://www.mingw-w64.org/)
[![Winsock2](https://img.shields.io/badge/Networking-Winsock2-orange.svg?style=flat-square)](https://docs.microsoft.com/en-us/windows/win32/winsock/)
[![Frontend](https://img.shields.io/badge/UI-HTML5%20%2F%20CSS3%20%2F%20SVG%20%2F%20JS-yellow.svg?style=flat-square)](frontend/)
[![Tests](https://img.shields.io/badge/Tests-8%20Passed-brightgreen.svg?style=flat-square)](tests/)

---

## 1. Project Introduction

The **Intelligent Disaster Evacuation Route Planner** is a high-performance, algorithm-driven emergency response system designed to compute safe, optimal evacuation corridors during urban catastrophes such as flash floods, chemical fires, industrial hazards, and structural collapses. 

The software models an entire city road network as an **Adjacency List Weighted Graph** in modern C++. It incorporates two distinct optimization strategies:
1. **Shortest Distance Routing (Standard Dijkstra)**: Minimizes physical travel distance ($km$) using a min-heap priority queue.
2. **Safety-Optimized Multi-Objective Routing (Risk-Weighted Dijkstra)**: Balances travel distance against physical hazard penalties (Low, Medium, High danger levels) with a user-tunable risk multiplier.
3. **Emergency Reachability Exploration (Breadth-First Search - BFS)**: Analyzes topological network connectivity to identify reachable zones and isolate disconnected city sectors.
4. **Dynamic Road Mutation**: Allows emergency dispatchers to dynamically block or reopen roads in real-time, instantly recalculating detours without application restarts.

The system features an **interactive SVG-based graphical dashboard** powered by a zero-dependency C++ HTTP REST server running natively on Windows.

> **Note**: This application uses simulated city data for academic and evaluation purposes and does not claim to deliver real-time emergency telemetry.

---

## 2. Problem Statement & Real-World Context

In catastrophic municipal crises:
- Traditional GPS navigators optimize strictly for travel time or shortest distance, frequently directing evacuees straight into flood zones, active fire corridors, or collapsed bridges.
- Critical access routes are frequently compromised dynamically as incidents escalate.
- Emergency responders require situational awareness to determine whether designated evacuation shelters are reachable from specific disaster epicenters.

### Objectives:
- Model a city network of 17 key urban sectors and 26 bidirectional roads.
- Implement graph algorithms independently in C++ STL without external algorithmic libraries.
- Formulate a quantifiable composite safety cost metric:
  $$\text{Route Cost} = \text{Distance} + (\text{Risk Penalty} \times W_{\text{risk}})$$
- Provide a clean, modern web interface with real-time SVG visualization and dynamic road toggles.

---

## 3. Technology Stack & Design Decisions

| Layer | Technology | Rationale & Academic Justification |
| :--- | :--- | :--- |
| **Backend Core** | C++ (C++14 Standard) | High computational efficiency, deterministic memory management, and standard STL containers (`vector`, `queue`, `priority_queue`, `unordered_map`). |
| **Data Structures** | Graph Adjacency List | Space-efficient representation ($O(V + E)$ vs $O(V^2)$ matrix) suited for sparse road topologies. |
| **Networking** | Native Windows Winsock2 (`ws2_32`) | Zero external runtime dependencies; natively compiles on MinGW GCC without requiring POSIX thread workarounds. |
| **Data Ingestion** | JSON (`nlohmann/json` v3.11) | Human-readable city topology specification (`city_data.json`). |
| **Frontend UI** | HTML5, CSS3, JavaScript | Clean, minimal light-theme GIS workstation dashboard with responsive metrics panels. |
| **Map Rendering** | Native SVG (Scalable Vector Graphics) | Dynamic vector map supporting live line highlights, animated route pulses, and click-to-block road interactions. |

---

## 4. ADSA Concepts & Mathematical Formulations

### A. Graph Representation (Adjacency List)
A graph $G = (V, E)$ consists of vertices $V$ (city locations) and edges $E$ (bidirectional road segments).
- An **Adjacency Matrix** requires $O(V^2)$ memory ($17^2 = 289$ cells), which is wasteful for planar road networks where each intersection connects to only 2–4 adjacent roads.
- An **Adjacency List** stores for each vertex $u$ a dynamic list of incident edges:
  ```cpp
  std::vector<std::vector<Edge>> adj;
  ```
  This consumes only $O(V + E)$ space and enables iteration over neighboring nodes in $O(\text{deg}(u))$ time.

### B. Dijkstra's Algorithm with Min-Heap Priority Queue
Dijkstra's algorithm computes the single-source shortest path on non-negative weighted graphs:
1. Initialize distance array $\text{dist}[v] = \infty$ for all $v \in V$, and $\text{dist}[\text{source}] = 0$.
2. Push $(0, \text{source})$ into a min-heap priority queue (`std::priority_queue` with `std::greater`).
3. While the heap is non-empty:
   - Extract vertex $u$ with minimum tentative distance.
   - If already visited or settled, skip.
   - For each incident edge $(u, v)$ with weight $w$:
     - If the road is marked `is_blocked`, **strictly skip the edge**.
     - If $\text{dist}[u] + w < \text{dist}[v]$, relax the edge: $\text{dist}[v] = \text{dist}[u] + w$, update parent pointer $\text{parent}[v] = u$, and push $(\text{dist}[v], v)$ into heap.
4. Backtrack from $\text{destination}$ through parent pointers to reconstruct the path.

### C. Multi-Objective Safety Cost Optimization
To prevent evacuees from entering hazardous flood or fire corridors, each road segment is annotated with a qualitative hazard score translated into a quantitative penalty:
- **Low Risk**: Penalty = $1$
- **Medium Risk**: Penalty = $3$
- **High Risk**: Penalty = $6$

The composite edge cost function is:
$$C(e) = \text{Distance}(e) + (\text{Risk Penalty}(e) \times W_{\text{risk}})$$
where $W_{\text{risk}} \ge 0$ is a user-adjustable safety multiplier.

**Mathematical Proof of Avoidance**:
Consider a high-risk direct shortcut of $1.5\text{ km}$ (Risk: High = 6) versus a safe detour of $4.7\text{ km}$ (Risk: Low = 2 total across 2 segments):
- At $W_{\text{risk}} = 0$ (Shortest Path):
  - Shortcut Cost: $1.5 + (6 \times 0) = 1.5$ (Chosen)
  - Detour Cost: $4.7 + (2 \times 0) = 4.7$
- At $W_{\text{risk}} = 2.0$ (Safest Path):
  - Shortcut Cost: $1.5 + (6 \times 2.0) = 13.5$
  - Detour Cost: $4.7 + (2 \times 2.0) = 8.7$ (Chosen!)
The algorithm automatically detours around dangerous shortcuts while tracking **actual physical distance** and **accumulated hazard penalty** separately.

### D. Breadth-First Search (BFS) for Reachability Analysis
BFS explores vertices level-by-level using a FIFO queue (`std::queue<int>`):
1. Enqueue source vertex and mark as visited.
2. Dequeue front vertex $u$, record exploration sequence.
3. For each neighbor $v$ across unblocked roads:
   - If not visited, mark visited and enqueue $v$.
4. Detects isolated partitions if $\text{reachable\_count} < |V|$.

---

## 5. Complexity Analysis

| Algorithm / Operation | Time Complexity | Auxiliary Space Complexity | Explanation |
| :--- | :--- | :--- | :--- |
| **Graph Construction** | $O(V + E)$ | $O(V + E)$ | Ingests vertices and edges into adjacency list. |
| **Dijkstra's Algorithm** | $O((V + E) \log V)$ | $O(V + E)$ | Each vertex is popped once ($O(V \log V)$); each edge is relaxed at most once ($O(E \log V)$). |
| **Safest Route Dijkstra** | $O((V + E) \log V)$ | $O(V + E)$ | Same min-heap relaxation with composite scalar weights. |
| **Breadth-First Search** | $O(V + E)$ | $O(V)$ | Every reachable node and edge is examined once. |
| **Road Block / Reopen** | $O(\text{deg}(u) + \text{deg}(v))$ | $O(1)$ | Direct index lookup via hash map + edge update. |
| **Network Reset** | $O(V + E)$ | $O(1)$ | Restores backup baseline snapshot. |

---

## 6. Project Directory Structure

```text
ADSA/
|-- backend/
|   |-- include/
|   |   |-- json.hpp               # nlohmann/json single header v3.11.2
|   |-- Graph.h                    # Adjacency list declarations & structures
|   |-- Graph.cpp                  # Graph implementation, dynamic mutation
|   |-- Algorithms.h               # Dijkstra, Safety-Weighted, BFS headers
|   |-- Algorithms.cpp             # Algorithm implementations & telemetry
|   |-- HttpServer.h               # Lightweight Winsock2 HTTP Server header
|   |-- HttpServer.cpp             # Winsock HTTP engine, multi-client routing
|   |-- api.h                      # REST API controller definitions
|   |-- api.cpp                    # REST API route handlers & JSON responses
|   |-- main.cpp                   # Server entrypoint & initialization
|
|-- frontend/
|   |-- index.html                 # Emergency dispatch dashboard UI
|   |-- style.css                  # Dark navy styling & responsive CSS
|   |-- script.js                  # Frontend controller & dynamic SVG engine
|
|-- data/
|   |-- city_data.json             # 17 locations, 26 roads simulated dataset
|
|-- tests/
|   |-- algorithm_tests.cpp        # Standalone ADSA unit verification suite
|
|-- bin/
|   |-- evac_server.exe            # Compiled backend server executable
|   |-- algorithm_tests.exe        # Compiled test suite executable
|
|-- build.bat                      # Windows compilation script
|-- run.bat                        # Windows execution & browser launcher
|-- .gitignore                     # Git tracking exclusions
|-- README.md                      # Comprehensive academic documentation
```

---

## 7. Installation & Compilation Instructions

### Prerequisites
- **Operating System**: Windows 10 or Windows 11.
- **Compiler**: MinGW GCC (g++ supporting C++14 or higher).
- **Libraries**: Winsock2 (`-lws2_32` included with all Windows MinGW installations).

### Step 1: Verify MinGW g++
Open a PowerShell or Command Prompt terminal and run:
```powershell
g++ --version
```
If `g++` is recognized, proceed to Step 2. If not, add `C:\MinGW\bin` (or your MinGW-w64 bin directory) to your Windows System `PATH`.

### Step 2: One-Click Build
Double-click `build.bat` in the project root folder, or execute:
```cmd
build.bat
```
This script will:
1. Compile the algorithm validation suite into `bin/algorithm_tests.exe`.
2. Compile the evacuation backend server into `bin/evac_server.exe`.
3. Automatically run the 8 test cases and report verification results.

### Manual Compilation Commands (Optional)
If you wish to compile directly from the terminal:
```powershell
# Compile Test Suite
g++ -std=c++14 -O2 -Ibackend/include backend/Graph.cpp backend/Algorithms.cpp tests/algorithm_tests.cpp -o bin/algorithm_tests.exe

# Compile Evacuation Server
g++ -std=c++14 -O2 -Ibackend/include backend/main.cpp backend/Graph.cpp backend/Algorithms.cpp backend/HttpServer.cpp backend/api.cpp -o bin/evac_server.exe -lws2_32
```

---

## 8. How to Run the Project

### Option A: Using the Launcher Script (Recommended)
Simply double-click `run.bat` or run:
```cmd
run.bat
```
This starts the backend on port 8080 and automatically opens `http://localhost:8080` in your default browser.

### Option B: Running from Terminal
```powershell
.\bin\evac_server.exe 8080
```
Then open any web browser and navigate to:
```text
http://localhost:8080
```

### Running the Project Using Git Bash (Windows)

#### 1. Prerequisites
- Windows 10/11
- Git Bash
- MinGW-w64 / GCC (`g++`)
- C++ compiler accessible through Windows `PATH`

#### 2. Clone the repository
```bash
git clone https://github.com/aaryan20-sus/disaster-evacuation-route-planner.git
```

#### 3. Navigate to the project root folder
```bash
cd disaster-evacuation-route-planner
```

#### 4. Build the project
```bash
cmd.exe /c build.bat
```
This compiles the C++ backend, builds the algorithm test executable (`bin/algorithm_tests.exe`), and automatically runs the 8 algorithm validation test cases.

#### 5. Run the application
```bash
cmd.exe /c run.bat
```
This starts the backend server on port `8080` and opens the application in the default web browser.

#### 6. Open manually if needed
```text
http://localhost:8080
```

#### Alternative: Run the backend directly
```bash
./bin/evac_server.exe 8080
```
Keep the terminal open while using the application.

### Troubleshooting
- **If `g++` is not recognized:** Install MinGW-w64 and add its `bin` folder to your Windows `PATH`.
- **If the executable is missing:** Run `cmd.exe /c build.bat` first to compile `bin/evac_server.exe` and `bin/algorithm_tests.exe`.
- **If port `8080` is occupied:** Stop the other server process using port `8080` before launching the backend.
- **If the browser does not open automatically:** Open your browser manually and visit `http://localhost:8080`.

---

## 9. Simulated City Topology

The network models a metropolitan emergency grid featuring:
- **3 Fortified Emergency Shelters**:
  - `SHELTER_A`: Emergency Shelter A (West Safe Bunker, capacity: 3,000)
  - `SHELTER_B`: Emergency Shelter B (Northeast Highland Relief Center, capacity: 2,500)
  - `SHELTER_C`: Emergency Shelter C (Southeast Stadium Camp, capacity: 3,200)
- **14 Urban Infrastructure Hubs**:
  - City Hospital, Fire Station, Police Headquarters, Central Park, Railway Station, Bus Terminal, City School, Shopping Mall, Old Market, Residential North, Residential South, Community Hall, Industrial Area, Tech Park.
- **26 Interconnecting Roads**:
  - Distances range from $1.4\text{ km}$ to $3.2\text{ km}$.
  - Realistic road hazards (e.g. `Central Alley (Flood Zone)` with High Risk, `River Bridge (Fire Risk)` with High Risk).

---

## 10. Automated Test Results

The test suite validates 8 distinct algorithmic scenarios (`bin/algorithm_tests.exe`):

```text
=== RUNNING ADSA ALGORITHM VALIDATION SUITE ===
[CityGraph] Successfully loaded 17 locations and 26 roads.

======================================================
 TEST CASE: 1. Dijkstra Shortest Route Calculation
======================================================
Status: SUCCESS
Source: City Hospital -> Dest: Emergency Shelter A
Distance: 5.1 km
Path: City Hospital -> Railway Station -> Emergency Shelter A
-> TEST 1 PASSED!

======================================================
 TEST CASE: 2. Safer Alternative Route vs Hazardous Shortcut
======================================================
Shortest Route Path: Central Park -> Market | Distance: 1.5 km | Risk: 6
Safest Route Path: Central Park -> Shopping Mall -> Market | Distance: 4.7 km | Risk: 2
-> TEST 2 PASSED! Safest route smartly avoided high-risk corridor.

======================================================
 TEST CASE: 3. Road Blocking & Dynamic Recalculation
======================================================
Road R01 (Hospital Link Way) is now marked BLOCKED.
New detour distance: 9.3 km (Detoured successfully)
-> TEST 3 PASSED!

======================================================
 TEST CASE: 4. Reopening Road & Restoring Route
======================================================
Road R01 reopened.
Distance restored to original optimal: 5.1 km
-> TEST 4 PASSED!

======================================================
 TEST CASE: 5. Breadth-First Search (BFS) Traversal
======================================================
Total nodes reached from City Hospital: 17
Traversal Order: City Hospital -> Railway Station -> Fire Station -> Central Park -> ...
-> TEST 5 PASSED!

======================================================
 TEST CASE: 6. Handling Disconnected / Unreachable Locations
======================================================
Blocked all ingress roads (R13, R14) to Emergency Shelter B.
Expected Failure Received: No available evacuation route found.
-> TEST 6 PASSED!

======================================================
 TEST CASE: 7. Handling Invalid Location Inputs
======================================================
Error captured: Error: Invalid source or destination location ID provided.
-> TEST 7 PASSED!

======================================================
 TEST CASE: 8. Resetting City Network
======================================================
Shelter B is reachable again after reset! Distance: 9 km
-> TEST 8 PASSED!

======================================================
 ALL 8 / 8 ADSA ALGORITHM TESTS PASSED PERFECTLY!
======================================================
```

---

## 11. REST API Reference

| Method | Endpoint | Description | Sample Request / Response |
| :--- | :--- | :--- | :--- |
| `GET` | `/api/network` | Retrieves entire graph JSON (nodes, roads, metadata). | `curl http://localhost:8080/api/network` |
| `GET` | `/api/locations` | Returns list of all 17 city sectors. | `curl http://localhost:8080/api/locations` |
| `GET` | `/api/roads` | Returns list of all 26 roads with blockage status. | `curl http://localhost:8080/api/roads` |
| `POST` | `/api/route/shortest`| Calculates shortest route (Dijkstra). | `{"source": 0, "destination": 14}` |
| `POST` | `/api/route/safest`  | Calculates risk-weighted safest route. | `{"source": 0, "destination": 14, "risk_weight": 2.5}` |
| `POST` | `/api/bfs`           | Performs BFS traversal from source. | `{"source": 0}` |
| `POST` | `/api/roads/block`   | Dynamically blocks a road. | `{"road_id": "R01"}` |
| `POST` | `/api/roads/reopen`  | Dynamically reopens a blocked road. | `{"road_id": "R01"}` |
| `POST` | `/api/reset`         | Restores original network topology. | `curl -X POST http://localhost:8080/api/reset` |

---

## 12. University Viva Preparation Guide (Q&A)

Here are the most common questions professors ask during an ADSA project viva and how to answer them:

### Q1: Why did you choose an Adjacency List over an Adjacency Matrix?
**Answer**: In urban road networks, graphs are planar and sparse—each intersection has only a few connecting roads. An Adjacency Matrix takes $O(V^2)$ memory and requires $O(V)$ iterations just to find the neighbors of a node. An Adjacency List takes only $O(V + E)$ space, and iterating over adjacent edges takes $O(\text{deg}(u))$ time, drastically speeding up Dijkstra's algorithm.

### Q2: How does Dijkstra's algorithm work and why do we use a priority queue?
**Answer**: Dijkstra's algorithm solves the single-source shortest path problem on graphs with non-negative edge weights. Using a naive array to find the minimum distance node takes $O(V^2)$ time. By utilizing a min-heap priority queue (`std::priority_queue`), extracting the minimum distance node takes $O(\log V)$, reducing overall complexity to $O((V + E) \log V)$.

### Q3: How do you handle road blockages in C++?
**Answer**: Each road edge contains a boolean flag `is_blocked`. When Dijkstra relaxes outgoing edges from node $u$, it first checks `if (edge.is_blocked) continue;`. When a road is blocked via the REST API, the road's `is_blocked` flag is set to `true` in both directions in $O(1)$ amortized time. Subsequent route queries naturally avoid the road and find detours.

### Q4: What is the purpose of Breadth-First Search (BFS) in this project?
**Answer**: While Dijkstra finds the shortest weighted path between two specific points, BFS explores the network layer-by-layer to assess overall reachability. In an emergency, BFS identifies all sectors that can still be reached from a dispatch center using currently open roads, and immediately flags completely cut-off zones.

### Q5: How is safety mathematically balanced against travel distance?
**Answer**: We formulate a multi-objective cost function: $\text{Cost} = \text{Distance} + (\text{Risk Penalty} \times W_{\text{risk}})$. Low risk roads have a penalty of 1, Medium risk has 3, and High risk has 6. By adjusting $W_{\text{risk}}$, the user controls how aggressively the planner steers away from hazardous zones (like flood zones or fire hazards), even if the hazardous path is physically shorter.

---

## 13. Future Improvements
1. **Dynamic Edge Speeds**: Support time-varying vehicular congestion based on crowd evacuation volume.
2. **Turn Penalties & Elevation**: Incorporate topological slope data for flood runoff modeling.
3. **Multi-Source Evacuation**: Automatically match evacuees to the nearest available shelter based on real-time shelter capacity constraints.
4. **Mobile GPS Integration**: Real-time geolocation tracking using progressive web app (PWA) standards.