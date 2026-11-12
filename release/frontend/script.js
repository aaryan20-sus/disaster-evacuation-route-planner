/**
 * Emergency Route Planner - Professional GIS Frontend Controller
 * Connects directly to C++ HTTP Backend Engine
 */

const API_BASE = window.location.origin;

// Application State Store
let cityLocations = [];
let cityRoads = [];
let locationMap = new Map(); // id -> location object
let roadMap = new Map();     // id -> road object
let activeRouteData = null;
let currentFilter = "all";
let currentSearchTerm = "";
let showRoadLabels = true;

// DOM Elements
const systemStatusEl = document.getElementById("systemStatus");
const selectSource = document.getElementById("selectSource");
const selectDestination = document.getElementById("selectDestination");
const sliderRiskWeight = document.getElementById("sliderRiskWeight");
const riskWeightValue = document.getElementById("riskWeightValue");
const riskWeightGroup = document.getElementById("riskWeightGroup");
const roadTableBody = document.getElementById("roadTableBody");
const roadCountBadge = document.getElementById("roadCountBadge");
const mapNetworkStats = document.getElementById("mapNetworkStats");
const alertBanner = document.getElementById("alertBanner");
const alertTitle = document.getElementById("alertTitle");
const alertMessage = document.getElementById("alertMessage");

// Telemetry Elements
const kpiDistance = document.getElementById("kpiDistance");
const kpiTime = document.getElementById("kpiTime");
const kpiRisk = document.getElementById("kpiRisk");
const kpiHops = document.getElementById("kpiHops");
const safetyGradeBadge = document.getElementById("safetyGradeBadge");
const pathTimeline = document.getElementById("pathTimeline");
const specTimeComp = document.getElementById("specTimeComp");
const specSpaceComp = document.getElementById("specSpaceComp");
const specVisitedNodes = document.getElementById("specVisitedNodes");
const specCompositeCost = document.getElementById("specCompositeCost");
const algoExplanation = document.getElementById("algoExplanation");
const badgeAlgoType = document.getElementById("badgeAlgoType");

// SVG Layer Groups
const roadsLayerBase = document.getElementById("roadsLayerBase");
const activeRouteLayer = document.getElementById("activeRouteLayer");
const roadLabelsLayer = document.getElementById("roadLabelsLayer");
const nodesLayer = document.getElementById("nodesLayer");

// Initialize on page load
window.addEventListener("DOMContentLoaded", () => {
  initThemeToggle();
  initNetwork();
  document.getElementById("btnQuickReset").addEventListener("click", resetNetwork);
});

/**
 * Theme Toggle Controller (Light / Dark Mode with localStorage persistence)
 */
function initThemeToggle() {
  const toggleBtn = document.getElementById("btnThemeToggle");
  const toggleLabel = document.getElementById("themeToggleLabel");
  if (!toggleBtn) return;

  const syncThemeUI = (theme) => {
    const isDark = theme === "dark";
    document.documentElement.setAttribute("data-theme", theme);
    toggleBtn.setAttribute("aria-pressed", String(isDark));
    toggleBtn.setAttribute("aria-label", isDark ? "Switch to light mode" : "Switch to dark mode");
    if (toggleLabel) {
      toggleLabel.textContent = isDark ? "Light" : "Dark";
    }
  };

  const currentTheme = document.documentElement.getAttribute("data-theme") === "dark" ? "dark" : "light";
  syncThemeUI(currentTheme);

  toggleBtn.addEventListener("click", () => {
    const isCurrentlyDark = document.documentElement.getAttribute("data-theme") === "dark";
    const nextTheme = isCurrentlyDark ? "light" : "dark";
    syncThemeUI(nextTheme);
    try {
      localStorage.setItem("evac-theme", nextTheme);
    } catch (e) {}
  });
}

/**
 * Ingest City Network data from C++ Backend API
 */
async function initNetwork() {
  try {
    const res = await fetch(`${API_BASE}/api/network`);
    if (!res.ok) throw new Error(`HTTP Error: ${res.status}`);
    const data = await res.json();

    cityLocations = data.locations || [];
    cityRoads = data.roads || [];

    locationMap.clear();
    cityLocations.forEach(loc => locationMap.set(loc.id, loc));

    roadMap.clear();
    cityRoads.forEach(r => roadMap.set(r.id, r));

    updateStatus(true);
    populateDropdowns();
    renderRoadTable();
    renderSvgMap();

    mapNetworkStats.innerText = `${cityLocations.length} Sectors • ${cityRoads.length} Roads`;
    roadCountBadge.innerText = cityRoads.length;

    // Automatically calculate default demo route: City Hospital -> Shelter A
    if (selectSource.children.length > 0 && selectDestination.children.length > 0) {
      selectSource.value = "0";      // City Hospital
      selectDestination.value = "14"; // Emergency Shelter A
      calculateRoute(false);
    }
  } catch (err) {
    console.error("Failed to connect to C++ backend:", err);
    updateStatus(false);
    showAlert("warning", "Backend Connection Offline", 
      "Could not reach C++ server at " + API_BASE + ". Ensure bin/evac_server.exe is running on port 8080.");
  }
}

function updateStatus(online) {
  if (online) {
    systemStatusEl.className = "connection-status";
    systemStatusEl.innerHTML = `<span class="status-indicator-dot"></span><span class="status-label">C++ Engine: Connected</span>`;
  } else {
    systemStatusEl.className = "connection-status offline";
    systemStatusEl.innerHTML = `<span class="status-indicator-dot"></span><span class="status-label">C++ Engine: Offline</span>`;
  }
}

/**
 * Populate Origin and Destination dropdown menus
 */
function populateDropdowns() {
  selectSource.innerHTML = "";
  selectDestination.innerHTML = "";

  const defaultSrcOpt = document.createElement("option");
  defaultSrcOpt.value = "";
  defaultSrcOpt.disabled = true;
  defaultSrcOpt.innerText = "-- Select Starting Origin --";
  selectSource.appendChild(defaultSrcOpt);

  const defaultDstOpt = document.createElement("option");
  defaultDstOpt.value = "";
  defaultDstOpt.disabled = true;
  defaultDstOpt.innerText = "-- Select Evacuation Shelter --";
  selectDestination.appendChild(defaultDstOpt);

  // Prioritize shelters in destination
  const sheltersGroup = document.createElement("optgroup");
  sheltersGroup.label = "Designated Emergency Shelters";

  const allLocationsGroup = document.createElement("optgroup");
  allLocationsGroup.label = "Other Municipal Facilities";

  cityLocations.forEach(loc => {
    // Populate Source
    const optSrc = document.createElement("option");
    optSrc.value = loc.id;
    optSrc.innerText = `${loc.name} (${loc.type})`;
    selectSource.appendChild(optSrc);

    // Populate Destination
    const optDst = document.createElement("option");
    optDst.value = loc.id;
    optDst.innerText = loc.name;

    if (loc.is_shelter) {
      sheltersGroup.appendChild(optDst);
    } else {
      allLocationsGroup.appendChild(optDst);
    }
  });

  selectDestination.appendChild(sheltersGroup);
  selectDestination.appendChild(allLocationsGroup);

  // Selection change handlers
  selectSource.onchange = () => { highlightSelectedNodes(); if (activeRouteData) calculateRoute(false); };
  selectDestination.onchange = () => { highlightSelectedNodes(); if (activeRouteData) calculateRoute(false); };
}

/**
 * Update UI when strategy radio toggle changes
 */
function updateStrategyUI() {
  const isSafest = document.querySelector('input[name="routeStrategy"]:checked').value === "safest";
  document.getElementById("labelSafest").classList.toggle("active", isSafest);
  document.getElementById("labelShortest").classList.toggle("active", !isSafest);
  riskWeightGroup.style.display = isSafest ? "flex" : "none";
  badgeAlgoType.innerText = isSafest ? "Safest Dijkstra" : "Shortest Dijkstra";

  if (activeRouteData) {
    calculateRoute(false);
  }
}

function updateRiskWeightValue(val) {
  riskWeightValue.innerText = `${parseFloat(val).toFixed(1)}x`;
  if (activeRouteData) {
    calculateRoute(false);
  }
}

/**
 * Render Interactive Clean Light SVG Map
 */
function renderSvgMap() {
  roadsLayerBase.innerHTML = "";
  activeRouteLayer.innerHTML = "";
  roadLabelsLayer.innerHTML = "";
  nodesLayer.innerHTML = "";

  // 1. Render Roads (Edges)
  cityRoads.forEach(road => {
    const uNode = locationMap.get(road.u);
    const vNode = locationMap.get(road.v);
    if (!uNode || !vNode) return;

    // Base Road Line
    const line = document.createElementNS("http://www.w3.org/2000/svg", "line");
    line.setAttribute("x1", uNode.x);
    line.setAttribute("y1", uNode.y);
    line.setAttribute("x2", vNode.x);
    line.setAttribute("y2", vNode.y);
    line.id = `svg-road-${road.id}`;

    let roadClass = "svg-road-base ";
    if (road.is_blocked) {
      roadClass += "svg-road-blocked";
    } else if (road.risk_level === "High") {
      roadClass += "svg-road-high";
    } else if (road.risk_level === "Medium") {
      roadClass += "svg-road-med";
    } else {
      roadClass += "svg-road-low";
    }
    line.setAttribute("class", roadClass);

    // Interactive road click: toggles block/reopen directly on map
    line.onclick = () => toggleRoadBlock(road.id);

    const titleEl = document.createElementNS("http://www.w3.org/2000/svg", "title");
    titleEl.textContent = `${road.name} (${road.id})\nDistance: ${road.distance_km} km | Risk: ${road.risk_level}\nStatus: ${road.is_blocked ? "BLOCKED" : "OPEN"}\nClick to toggle status`;
    line.appendChild(titleEl);

    roadsLayerBase.appendChild(line);

    // Midpoint Distance Badge
    if (showRoadLabels) {
      const midX = (uNode.x + vNode.x) / 2;
      const midY = (uNode.y + vNode.y) / 2;

      const badgeGroup = document.createElementNS("http://www.w3.org/2000/svg", "g");
      badgeGroup.setAttribute("class", road.is_blocked ? "road-label-badge is-blocked" : "road-label-badge");
      badgeGroup.onclick = (e) => { e.stopPropagation(); toggleRoadBlock(road.id); };

      const badgeW = road.is_blocked ? 44 : 30;
      const rect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
      rect.setAttribute("x", midX - badgeW / 2);
      rect.setAttribute("y", midY - 8.5);
      rect.setAttribute("width", badgeW);
      rect.setAttribute("height", 17);
      rect.setAttribute("class", "road-label-rect");

      const text = document.createElementNS("http://www.w3.org/2000/svg", "text");
      text.setAttribute("x", midX);
      text.setAttribute("y", midY);
      text.setAttribute("class", "road-label-text");
      text.textContent = road.is_blocked ? "CLOSED" : `${road.distance_km}k`;

      badgeGroup.appendChild(rect);
      badgeGroup.appendChild(text);
      roadLabelsLayer.appendChild(badgeGroup);
    }
  });

  // 2. Render Nodes (Vertices)
  cityLocations.forEach(node => {
    const g = document.createElementNS("http://www.w3.org/2000/svg", "g");
    const roleClass = node.is_shelter
      ? "node-role-shelter"
      : node.type === "hospital"
        ? "node-role-hospital"
        : (node.type === "fire_station" || node.type === "police")
          ? "node-role-emergency"
          : "node-role-default";
    g.setAttribute("class", `svg-node-group ${roleClass}`);
    g.id = `svg-node-${node.id}`;
    g.setAttribute("transform", `translate(${node.x}, ${node.y})`);

    // Click handler to select node as origin or destination
    g.onclick = (e) => {
      e.stopPropagation();
      handleNodeClick(node.id);
    };

    // Generous transparent hit target for reliable mouse/touch tapping
    const hitTarget = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    hitTarget.setAttribute("r", 24);
    hitTarget.setAttribute("class", "node-hit-target");
    g.appendChild(hitTarget);

    // Inner icon wrapper so hover/BFS scaling happens at (0,0) without overriding translate(x,y)
    const iconWrap = document.createElementNS("http://www.w3.org/2000/svg", "g");
    iconWrap.setAttribute("class", "node-icon-wrap");

    // Shelter outer indicator ring
    if (node.is_shelter) {
      const ring = document.createElementNS("http://www.w3.org/2000/svg", "circle");
      ring.setAttribute("r", 23);
      ring.setAttribute("class", "node-shelter-ring");
      iconWrap.appendChild(ring);
    }

    // Node Circle (Crisp vector marker styled via role classes)
    const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    const radius = node.is_shelter ? 17 : 14;
    circle.setAttribute("r", radius);
    circle.setAttribute("class", "node-circle-outer");
    iconWrap.appendChild(circle);

    // Crisp monochrome SVG vector icon (non-blocking for pointer events)
    iconWrap.appendChild(createNodeVectorIcon(node));
    g.appendChild(iconWrap);

    // Cartographic Haloed Label
    const labelY = radius + 12;
    const labelText = document.createElementNS("http://www.w3.org/2000/svg", "text");
    labelText.setAttribute("x", 0);
    labelText.setAttribute("y", labelY);
    labelText.setAttribute("class", "node-label-text");
    labelText.textContent = node.name;
    g.appendChild(labelText);

    // Tooltip
    const titleEl = document.createElementNS("http://www.w3.org/2000/svg", "title");
    titleEl.textContent = `${node.name} (${node.code})\n${node.description}\nCapacity: ${node.capacity} people\nClick to set as ${node.is_shelter ? "Destination Shelter" : "Origin Location"}`;
    g.appendChild(titleEl);

    nodesLayer.appendChild(g);
  });

  highlightSelectedNodes();
  if (activeRouteData && activeRouteData.success) {
    drawActiveRoute(activeRouteData);
  }
}

/**
 * Create a crisp vector icon inside a centered 16x16 (-8..8) SVG group
 */
function createNodeVectorIcon(node) {
  const svgNS = "http://www.w3.org/2000/svg";
  const iconGroup = document.createElementNS(svgNS, "g");
  iconGroup.setAttribute("class", "node-glyph");
  iconGroup.setAttribute("pointer-events", "none");

  const svg = document.createElementNS(svgNS, "svg");
  const size = node.is_shelter ? 16 : 14;
  svg.setAttribute("x", -size / 2);
  svg.setAttribute("y", -size / 2);
  svg.setAttribute("width", size);
  svg.setAttribute("height", size);
  svg.setAttribute("viewBox", "0 0 24 24");
  svg.setAttribute("fill", "none");
  svg.setAttribute("stroke", "currentColor");
  svg.setAttribute("stroke-width", "2.1");
  svg.setAttribute("stroke-linecap", "round");
  svg.setAttribute("stroke-linejoin", "round");

  let markup = "";
  if (node.is_shelter) {
    markup = '<path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>';
  } else if (node.type === "hospital") {
    markup = '<path d="M12 5v14M5 12h14"/>';
  } else if (node.type === "fire_station") {
    markup = '<path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.07-2.14-.22-4.05 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.15.43-2.29 1-3a2.5 2.5 0 0 0 2.5 2.5z"/>';
  } else if (node.type === "police") {
    markup = '<polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/>';
  } else if (node.type === "station") {
    markup = '<rect x="4" y="3" width="16" height="14" rx="2"/><path d="M4 11h16M12 3v8M8 19l-2 3M16 19l2 3"/>';
  } else if (node.type === "school") {
    markup = '<path d="M22 10v6M2 10l10-5 10 5-10 5z"/><path d="M6 12v5c3 3 9 3 12 0v-5"/>';
  } else if (node.type === "park") {
    markup = '<path d="M12 3l6 8h-3.5L19 18H5l4.5-7H6l6-8z"/><path d="M12 18v4"/>';
  } else if (node.type === "commercial") {
    markup = '<path d="M3 9l2-4h14l2 4v11a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V9z"/><path d="M3 9h18M9 21V9"/>';
  } else if (node.type === "industrial") {
    markup = '<path d="M2 20h20V9l-5 3V9l-5 3V4H2v16z"/>';
  } else {
    markup = '<path d="M3 10.5L12 3l9 7.5V20a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1v-9.5z"/>';
  }
  svg.innerHTML = markup;
  iconGroup.appendChild(svg);
  return iconGroup;
}

/**
 * Handle clicking on a node directly in SVG canvas
 */
function handleNodeClick(nodeId) {
  const clickedNode = locationMap.get(nodeId);
  const idStr = String(nodeId);

  if (clickedNode && clickedNode.is_shelter) {
    // Tapping a Shelter sets it as Destination (unless Origin is empty)
    if (!selectSource.value) {
      selectSource.value = idStr;
    } else if (selectSource.value !== idStr) {
      selectDestination.value = idStr;
    }
  } else {
    // Tapping a city location sets it as Origin (unless Destination is empty)
    if (!selectDestination.value) {
      selectDestination.value = idStr;
    } else if (selectDestination.value !== idStr) {
      selectSource.value = idStr;
    }
  }

  highlightSelectedNodes();
  if (selectSource.value && selectDestination.value) {
    calculateRoute();
  }
}

/**
 * Highlight active origin & destination in SVG
 */
function highlightSelectedNodes() {
  const srcId = selectSource.value;
  const dstId = selectDestination.value;

  document.querySelectorAll(".svg-node-group").forEach(el => {
    el.classList.remove("node-selected-origin", "node-selected-dest");
  });

  if (srcId) {
    const srcEl = document.getElementById(`svg-node-${srcId}`);
    if (srcEl) srcEl.classList.add("node-selected-origin");
  }

  if (dstId) {
    const dstEl = document.getElementById(`svg-node-${dstId}`);
    if (dstEl) dstEl.classList.add("node-selected-dest");
  }
}

/**
 * Calculate Route (Invoking C++ Backend Dijkstra Engine)
 */
async function calculateRoute(showSuccessAlert = true) {
  const srcId = selectSource.value;
  const dstId = selectDestination.value;

  if (!srcId || !dstId) {
    showAlert("warning", "Missing Selection", "Please specify both an Origin and an Evacuation Shelter.");
    return;
  }

  const strategy = document.querySelector('input[name="routeStrategy"]:checked').value;
  const riskWeight = parseFloat(sliderRiskWeight.value) || 2.0;

  const endpoint = strategy === "safest" ? `${API_BASE}/api/route/safest` : `${API_BASE}/api/route/shortest`;
  const payload = {
    source: parseInt(srcId),
    destination: parseInt(dstId),
    risk_weight: riskWeight
  };

  try {
    const res = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });

    const data = await res.json();
    activeRouteData = data;

    if (!data.success) {
      clearActiveRoute();
      showAlert("danger", "Evacuation Route Blocked", data.message || "All paths to the destination are obstructed.");
      displayNoRouteTelemetry(data);
      return;
    }

    // Success: Render path and update panels
    closeAlert();
    drawActiveRoute(data);
    updateTelemetryPanels(data);
  } catch (err) {
    console.error("Route calculation error:", err);
    showAlert("danger", "Computation Failure", "Failed to communicate with C++ routing engine.");
  }
}

/**
 * Draw Clean Solid Active Evacuation Path on SVG
 */
function drawActiveRoute(routeData) {
  activeRouteLayer.innerHTML = "";
  if (!routeData || !routeData.steps) return;

  // Outer casing pass for crisp separation over intersecting corridors
  routeData.steps.forEach(step => {
    const uNode = locationMap.get(step.from_id);
    const vNode = locationMap.get(step.to_id);
    if (!uNode || !vNode) return;

    const casing = document.createElementNS("http://www.w3.org/2000/svg", "line");
    casing.setAttribute("x1", uNode.x);
    casing.setAttribute("y1", uNode.y);
    casing.setAttribute("x2", vNode.x);
    casing.setAttribute("y2", vNode.y);
    casing.setAttribute("class", "svg-road-active-casing");
    activeRouteLayer.appendChild(casing);
  });

  routeData.steps.forEach(step => {
    const uNode = locationMap.get(step.from_id);
    const vNode = locationMap.get(step.to_id);
    if (!uNode || !vNode) return;

    const line = document.createElementNS("http://www.w3.org/2000/svg", "line");
    line.setAttribute("x1", uNode.x);
    line.setAttribute("y1", uNode.y);
    line.setAttribute("x2", vNode.x);
    line.setAttribute("y2", vNode.y);
    line.setAttribute("class", "svg-road-active");
    activeRouteLayer.appendChild(line);
  });
}

function clearActiveRoute() {
  activeRouteLayer.innerHTML = "";
}

/**
 * Populate Telemetry & Algorithm Cards
 */
function updateTelemetryPanels(data) {
  kpiDistance.innerHTML = `${data.total_distance_km.toFixed(1)} <small>km</small>`;
  kpiTime.innerHTML = `${data.estimated_time_minutes.toFixed(1)} <small>min</small>`;
  kpiRisk.innerText = data.total_risk_penalty;
  kpiHops.innerText = data.steps ? data.steps.length : 0;

  // Clean Safety Grade Badge
  safetyGradeBadge.innerText = data.safety_grade || "Calculated";
  if (data.total_risk_penalty <= 3) {
    safetyGradeBadge.className = "tag tag-safe";
  } else if (data.total_risk_penalty <= 8) {
    safetyGradeBadge.className = "tag tag-warning";
  } else {
    safetyGradeBadge.className = "tag tag-danger";
  }

  // Algorithm Specs
  specTimeComp.innerText = data.complexity_time || "O((V + E) log V)";
  specSpaceComp.innerText = data.complexity_space || "O(V + E)";
  specVisitedNodes.innerText = `${data.nodes_visited_count} / ${cityLocations.length}`;
  specCompositeCost.innerText = data.composite_cost.toFixed(2);
  algoExplanation.innerText = data.algorithm_explanation || "Optimal route computed by C++ Dijkstra min-heap.";

  // Turn-by-Turn Navigation List
  pathTimeline.innerHTML = "";
  if (data.steps && data.steps.length > 0) {
    data.steps.forEach((step, idx) => {
      const stepEl = document.createElement("div");
      stepEl.className = "turn-item";

      let riskTagClass = "tag-safe";
      if (step.risk_level === "Medium") riskTagClass = "tag-warning";
      if (step.risk_level === "High") riskTagClass = "tag-danger";

      stepEl.innerHTML = `
        <span class="turn-index">${idx + 1}</span>
        <div class="turn-details">
          <div class="turn-path-title">${step.from_name} &rarr; ${step.to_name}</div>
          <div class="turn-meta">Via ${step.road_name} (${step.road_id}) &bull; ${step.segment_distance} km &bull; <span class="tag ${riskTagClass}">${step.risk_level}</span></div>
        </div>
      `;
      pathTimeline.appendChild(stepEl);
    });
  } else {
    pathTimeline.innerHTML = `<p class="empty-hint">Origin and Destination are identical.</p>`;
  }
}

function displayNoRouteTelemetry(data) {
  kpiDistance.innerHTML = `-- <small>km</small>`;
  kpiTime.innerHTML = `-- <small>min</small>`;
  kpiRisk.innerText = "--";
  kpiHops.innerText = "--";

  safetyGradeBadge.innerText = "No Route";
  safetyGradeBadge.className = "tag tag-danger";

  specVisitedNodes.innerText = `${data.nodes_visited_count || 0} / ${cityLocations.length}`;
  specCompositeCost.innerText = "∞";
  algoExplanation.innerText = data.algorithm_explanation || data.message;

  pathTimeline.innerHTML = `
    <div class="empty-route-notice">
      All corridors to the selected shelter are currently blocked. Select an alternate shelter or reopen a corridor.
    </div>
  `;
}

/**
 * Execute Breadth-First Search (BFS) for Reachability Analysis
 */
async function executeBFS() {
  const srcId = selectSource.value || "0";
  try {
    const res = await fetch(`${API_BASE}/api/bfs`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ source: parseInt(srcId) })
    });

    const data = await res.json();
    if (!data.success) {
      showAlert("warning", "BFS Error", data.message);
      return;
    }

    // Display BFS Results
    document.getElementById("bfsReachableBadge").innerText = `${data.reachable_count} / ${cityLocations.length}`;
    const orderList = document.getElementById("bfsOrderList");
    orderList.innerHTML = "";

    data.traversal_order_names.forEach((name, idx) => {
      const chip = document.createElement("span");
      chip.className = "bfs-chip";
      chip.innerText = `${idx + 1}. ${name}`;
      orderList.appendChild(chip);
    });

    // Subtle animation of explored nodes on SVG
    animateBFSTraversal(data.traversal_order_ids);

    showAlert("success", "BFS Traversal Complete", 
      `Explored ${data.reachable_count} reachable sectors from ${data.source_name}. ${data.unreachable_count} sectors isolated.`);
  } catch (err) {
    console.error("BFS execution error:", err);
    showAlert("danger", "BFS Execution Failed", "Failed to run BFS on C++ backend.");
  }
}

function animateBFSTraversal(nodeIds) {
  nodeIds.forEach((nid, i) => {
    setTimeout(() => {
      const g = document.getElementById(`svg-node-${nid}`);
      if (g) {
        g.classList.add("node-bfs-active");
        setTimeout(() => {
          g.classList.remove("node-bfs-active");
        }, 320);
      }
    }, i * 120);
  });
}

/**
 * Dynamic Road Management (Block / Reopen)
 */
async function toggleRoadBlock(roadId) {
  const road = roadMap.get(roadId);
  if (!road) return;

  const action = road.is_blocked ? "reopen" : "block";
  const endpoint = `${API_BASE}/api/roads/${action}`;

  try {
    const res = await fetch(endpoint, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ road_id: roadId })
    });

    const data = await res.json();
    if (!data.success) {
      showAlert("danger", "Road Action Failed", data.error || "Failed to update road state.");
      return;
    }

    // Update local state
    road.is_blocked = (action === "block");
    renderSvgMap();
    renderRoadTable();

    showAlert(action === "block" ? "warning" : "success", 
      action === "block" ? "Road Blocked" : "Road Reopened",
      data.message);

    // Automatically recalculate active route if origin & destination are selected
    if (selectSource.value && selectDestination.value) {
      calculateRoute(false);
    }
  } catch (err) {
    console.error("Toggle road block error:", err);
    showAlert("danger", "Network Error", "Could not reach C++ backend to update road status.");
  }
}

/**
 * Reset City Network to Baseline Operational State
 */
async function resetNetwork() {
  try {
    const res = await fetch(`${API_BASE}/api/reset`, { method: "POST" });
    const data = await res.json();
    if (data.success) {
      showAlert("success", "City Network Reset", data.message);
      await initNetwork();
    }
  } catch (err) {
    console.error("Reset network error:", err);
    showAlert("danger", "Reset Failed", "Could not reset city road network.");
  }
}

/**
 * Render Roads Table in Management Panel (Exact 7 Columns)
 * Columns: Road Name | Starting Location | Ending Location | Distance | Risk Level | Status | Action
 */
function renderRoadTable() {
  roadTableBody.innerHTML = "";

  const filteredRoads = cityRoads.filter(r => {
    // Status / Risk Filter
    if (currentFilter === "blocked" && !r.is_blocked) return false;
    if (currentFilter === "High" && r.risk_level !== "High") return false;

    // Search query filter
    if (currentSearchTerm) {
      const term = currentSearchTerm.toLowerCase();
      const uName = locationMap.get(r.u)?.name.toLowerCase() || "";
      const vName = locationMap.get(r.v)?.name.toLowerCase() || "";
      const rName = r.name.toLowerCase();
      const rId = r.id.toLowerCase();
      if (!uName.includes(term) && !vName.includes(term) && !rName.includes(term) && !rId.includes(term)) {
        return false;
      }
    }
    return true;
  });

  if (filteredRoads.length === 0) {
    roadTableBody.innerHTML = `<tr><td colspan="7" class="empty-state">No roadways match the active filter.</td></tr>`;
    return;
  }

  filteredRoads.forEach(r => {
    const tr = document.createElement("tr");

    let riskClass = "risk-low";
    if (r.risk_level === "Medium") riskClass = "risk-medium";
    if (r.risk_level === "High") riskClass = "risk-high";

    const statusHtml = r.is_blocked
      ? `<span class="status-badge-inline blocked"><span class="status-dot-inline blocked"></span> Blocked</span>`
      : `<span class="status-badge-inline open"><span class="status-dot-inline open"></span> Open</span>`;

    const actionBtn = r.is_blocked
      ? `<button class="btn-table-action btn-table-reopen" onclick="toggleRoadBlock('${r.id}')">Reopen</button>`
      : `<button class="btn-table-action btn-table-block" onclick="toggleRoadBlock('${r.id}')">Block</button>`;

    const startLocName = locationMap.get(r.u)?.name || "Unknown";
    const endLocName = locationMap.get(r.v)?.name || "Unknown";

    tr.innerHTML = `
      <td class="road-name-cell">${r.name} <span class="road-id-pill">${r.id}</span></td>
      <td>${startLocName}</td>
      <td>${endLocName}</td>
      <td class="font-mono">${r.distance_km} km</td>
      <td><span class="risk-badge ${riskClass}">${r.risk_level}</span></td>
      <td>${statusHtml}</td>
      <td class="text-right">${actionBtn}</td>
    `;
    roadTableBody.appendChild(tr);
  });
}

function filterRoads(filter, btn) {
  currentFilter = filter;
  document.querySelectorAll(".filter-chip").forEach(b => b.classList.remove("active"));
  if (btn) btn.classList.add("active");
  renderRoadTable();
}

function handleRoadSearch(val) {
  currentSearchTerm = val;
  renderRoadTable();
}

function toggleRoadDistances() {
  showRoadLabels = !showRoadLabels;
  const btn = document.getElementById("btnToggleLabels");
  if (btn) btn.classList.toggle("is-active", showRoadLabels);
  renderSvgMap();
}

function resetMapZoom() {
  const svg = document.getElementById("citySvgMap");
  svg.setAttribute("viewBox", "0 0 1000 680");
}

/**
 * Alert Banner Helpers (Clean crisp SVG indicators)
 */
function showAlert(type, title, msg) {
  alertBanner.className = `alert-box alert-${type}`;
  if (type === "danger") {
    alertIcon.innerHTML = `<svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/></svg>`;
  } else if (type === "warning") {
    alertIcon.innerHTML = `<svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>`;
  } else {
    alertIcon.innerHTML = `<svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"/></svg>`;
  }
  alertTitle.innerText = title;
  alertMessage.innerText = msg;
}

function closeAlert() {
  alertBanner.className = "alert-box hidden";
}