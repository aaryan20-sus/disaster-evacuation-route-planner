# Intelligent Disaster Evacuation Route Planner (Windows Release)

This is the standalone, ready-to-run Windows package. **No C++ compiler (GCC/MinGW), Git Bash, or Command Prompt setup is required.**

## How to Run

1. Make sure you have **extracted** the entire ZIP archive (do not run `START.bat` directly from inside a ZIP preview window).
2. Double-click **`START.bat`**.
3. A console window will open, start the C++ evacuation backend on port `8080`, and automatically open your default browser at:
   ```text
   http://localhost:8080
   ```
4. **Keep the console window open** while using the web application.
5. To stop the application safely, press `Ctrl+C` inside the console window or close the console window.

## Package Contents

- `START.bat` — One-click Windows launcher
- `bin/evac_server.exe` — Precompiled standalone C++ backend server (statically linked; uses only built-in Windows DLLs `KERNEL32.dll`, `msvcrt.dll`, `WS2_32.dll`)
- `frontend/` — Web dashboard (`index.html`, `style.css`, `script.js`)
- `data/city_data.json` — Simulated city graph dataset (17 locations, 26 roads)
- `SHA256SUMS.txt` — SHA-256 checksum for integrity & IT verification

## Executable Verification (SHA-256)

- **File:** `bin/evac_server.exe`
- **SHA-256:** `326838907F7CDFD8AB0B0451F8C112A060F62F581E4F52369BAB9774568F8740`
- **Digital Signature Status:** Unsigned (student academic build compiled with MinGW GCC)

You or an IT administrator can verify the checksum locally in PowerShell or Command Prompt:

```powershell
Get-FileHash .\bin\evac_server.exe -Algorithm SHA256
```

```cmd
certutil -hashfile bin\evac_server.exe SHA256
```

## Managed Computers & Windows Device Guard Policy

If you are running this package on a university, corporate, or organization-managed Windows computer, Windows security policies (**Windows Defender Application Control / Device Guard** or **AppLocker**) may display an error such as:

> `evac_server.exe was blocked by your organization's Device Guard policy.`

### Why This Happens
Managed organizations frequently restrict Windows from running **unapproved or unsigned executables** downloaded from the internet—even when all runtime dependencies are statically linked. Commercial or self-signed code signing also does **not** guarantee execution on a managed computer if the organization's policy only trusts Microsoft-signed apps, its own internal certificate authority, or explicitly allowlisted file hashes.

### How to Request Approval from Your IT Administrator
Do **not** attempt to disable or bypass organization security controls. Instead, submit an allowlist request to your IT Helpdesk / administrator with the following verification details:

1. **Application:** `bin/evac_server.exe` (Intelligent Disaster Evacuation Route Planner — 3rd-semester ADSA academic project)
2. **SHA-256 Checksum:** `326838907F7CDFD8AB0B0451F8C112A060F62F581E4F52369BAB9774568F8740`
3. **Network & System Behavior:** Binds strictly to loopback `127.0.0.1:8080` (`http://localhost:8080`), makes no external internet connections, requires no administrator privileges, and writes no registry keys or system files.
4. **Source Transparency:** Provide your IT team with the GitHub repository link so they can audit `backend/` and rebuild from source with `build.bat` if required by policy.

If IT approval is not available on a locked-down machine, run the extracted `release/` folder on a **personal, unmanaged Windows 10/11 computer**.

## General Troubleshooting

- **Windows SmartScreen prompt on a personal PC ("Windows protected your PC"):** Click **More info** -> **Run anyway** (expected for unsigned binaries without SmartScreen reputation).
- **Port 8080 already in use:** Close any previously opened `START.bat` console window or other process listening on port `8080`, then double-click `START.bat` again.
- **Browser did not open automatically:** Keep the console window open and visit `http://localhost:8080` manually.
