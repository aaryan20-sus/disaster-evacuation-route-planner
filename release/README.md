# Intelligent Disaster Evacuation Route Planner (Windows Release)

A standalone Windows package for computing shortest, safety-weighted, and reachability-aware evacuation routes across a simulated city road network using a C++ graph engine (`Dijkstra` and `BFS`) and a browser-based interactive map.

**No C++ compiler (GCC/MinGW), Git Bash, or terminal setup is required.**

---

## System Requirements

- Windows 10 or Windows 11
- Any modern web browser (Edge, Chrome, Firefox)
- Free TCP port `8080` on `127.0.0.1` (`localhost`)
- No extra runtime installations needed (`bin/evac_server.exe` is statically linked and uses only built-in Windows DLLs: `KERNEL32.dll`, `msvcrt.dll`, `WS2_32.dll`)

---

## Download and Extraction

1. Download the release ZIP archive (`v1.0.0`) from the [GitHub Releases page](https://github.com/aaryan20-sus/disaster-evacuation-route-planner/releases/tag/v1.0.0).
2. Right-click the ZIP file and select **Extract All...** to extract all files into a normal folder.
   - Do **not** double-click `START.bat` directly inside a ZIP preview window.
3. Open the extracted folder. Make sure `START.bat`, `bin/`, `frontend/`, and `data/` are all present together.

---

## How to Launch

1. Double-click **`START.bat`**.
2. A console window opens, verifies required files and port `8080`, starts `bin\evac_server.exe`, and waits until the backend responds.
3. Your default web browser automatically opens:
   ```text
   http://localhost:8080
   ```
4. **Keep the console window open** while using the web application.

---

## How to Stop the Application

- Click inside the `START.bat` console window and press **`Ctrl+C`**, **or**
- Close the `START.bat` console window (`X` button).

This cleanly shuts down the local C++ backend server.

---

## Troubleshooting

- **Windows SmartScreen ("Windows protected your PC") on a personal computer:**
  Click **More info** -> **Run anyway**. This warning appears because the binary is an unsigned student build without Microsoft SmartScreen reputation.
- **Port 8080 is already in use:**
  Close any previously opened `START.bat` console window or stop the application using port `8080`, then double-click `START.bat` again.
- **Browser does not open automatically:**
  Confirm the `START.bat` console says the server is listening, then manually visit `http://localhost:8080`.
- **Missing files error (`bin\evac_server.exe`, `data\city_data.json`, or `frontend\index.html`):**
  Re-extract the entire ZIP archive so the folder hierarchy remains intact.

---

## Device Guard Restrictions & IT Approval

On university, corporate, or school-managed Windows devices, **Windows Defender Application Control (Device Guard)** or **AppLocker** may display:

> `evac_server.exe was blocked by your organization's Device Guard policy.`

### Why This Happens
Managed computers often block unapproved or unsigned executables downloaded from the internet. Even commercially signed executables are not guaranteed to run under strict enterprise allowlists unless explicitly approved by organization IT. Do **not** attempt to disable or bypass Windows security policies.

### How to Request IT Administrator Approval
Share the following details with your IT Helpdesk so they can inspect and allowlist the binary if permitted by policy:

- **Executable:** `bin/evac_server.exe` (32-bit Windows PE, compiled statically with MinGW GCC)
- **Digital Signature Status:** Unsigned (`NotSigned`)
- **SHA-256 Checksum:** `326838907F7CDFD8AB0B0451F8C112A060F62F581E4F52369BAB9774568F8740` (also recorded in [`SHA256SUMS.txt`](SHA256SUMS.txt))
- **Runtime Behavior:** Runs strictly locally on `127.0.0.1:8080`, requires no administrator elevation, writes no registry keys or system files, and makes no outbound internet requests.
- **Source Code:** Available for review on [GitHub](https://github.com/aaryan20-sus/disaster-evacuation-route-planner).

IT administrators or users can verify the checksum locally:

```powershell
Get-FileHash .\bin\evac_server.exe -Algorithm SHA256
```

```cmd
certutil -hashfile bin\evac_server.exe SHA256
```

If IT approval is not possible on a managed PC, run the extracted package on a personal, unmanaged Windows 10/11 computer.
