# Tap Click Counter Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a tiny Windows desktop app with a `+`/`−` counter that also increments on any window click, compiled to a small dependency-free `.exe`.

**Architecture:** Single `main.c` using the raw Win32 API (no external libraries, no comctl32), compiled with mingw-w64 `gcc`. One window, one static text display, two button controls, all state in file-static globals.

**Tech Stack:** C, Win32 API (`user32`, `gdi32` via `windows.h`), mingw-w64 `gcc` (already installed at `C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\gcc.exe`), Windows batch script for the build.

## Global Constraints

- No .NET, no Electron, no third-party libraries — Win32 API only, per the design's tech-stack decision.
- Build must produce `counter.exe` in the tens-of-KB range: `-mwindows -O2 -s`, no `comctl32` linkage (plain `BUTTON`/`STATIC` classes don't need it).
- Counter floors at 0 — the `−` button and any decrement path must never take it negative.
- Counter always starts at 0 on launch — no persistence, no file I/O at runtime.
- Clicking a `+`/`−` button must not also register as a window-background click (no double counting).
- No automated test framework (per spec) — verification is: (a) build succeeds, (b) the exe launches and stays running without crashing, checked via PowerShell process inspection after each task; full interactive click verification happens once, by the user, at the end of Task 3.

---

### Task 1: Minimal window + build scaffolding

**Files:**
- Create: `main.c`
- Create: `build.bat`
- Create: `.gitignore`
- Create: `README.md`

**Interfaces:**
- Produces: a `WndProc` function and `WinMain` entry point in `main.c` that Task 2 will extend with a `WM_CREATE` case and a `WM_COMMAND` case (both currently absent, `switch` only handles `WM_DESTROY`).
- Produces: `build.bat`, invoked identically by every later task to rebuild.

- [ ] **Step 1: Write `main.c` with a minimal window (no controls yet)**

```c
#include <windows.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "TapCounterWindowClass";

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, CLASS_NAME, "Tap Counter",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 260,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}
```

- [ ] **Step 2: Write `build.bat`**

```bat
@echo off
gcc main.c -o counter.exe -mwindows -O2 -s
if %ERRORLEVEL% EQU 0 (
    echo Build succeeded: counter.exe
) else (
    echo Build failed.
    exit /b 1
)
```

- [ ] **Step 3: Write `.gitignore`**

```
*.exe
*.o
*.obj
```

- [ ] **Step 4: Write `README.md`**

```markdown
# Tap Click Counter

A tiny Windows desktop app: a counter you increase by clicking `+`, the
window background, or decrease with `-` (floors at 0).

## Build

Requires `gcc` (mingw-w64) on your `PATH`.

```
build.bat
```

Produces `counter.exe` (no other files or runtime dependencies needed).

## Run

```
counter.exe
```
```

- [ ] **Step 5: Build and verify the exe is produced**

Run (PowerShell, from the repo root):
```powershell
.\build.bat
Test-Path .\counter.exe
```
Expected: `Build succeeded: counter.exe` printed, and `Test-Path` returns `True`.

- [ ] **Step 6: Launch and verify the process starts without crashing**

Run:
```powershell
$p = Start-Process -FilePath .\counter.exe -PassThru
Start-Sleep -Seconds 1
Get-Process -Id $p.Id -ErrorAction SilentlyContinue
Stop-Process -Id $p.Id -Force
```
Expected: `Get-Process` returns a running `counter` process entry (not empty/error) before it's stopped — confirms the window opened and didn't crash on startup.

- [ ] **Step 7: Commit**

```bash
git add main.c build.bat .gitignore README.md
git commit -m "Add minimal Win32 window scaffolding"
```

---

### Task 2: Counter display and +/− buttons

**Files:**
- Modify: `main.c` (full replacement shown below)

**Interfaces:**
- Consumes: `WndProc`/`WinMain` structure from Task 1.
- Produces: `UpdateCounterText(void)` — repaints `g_hStatic` from `g_counter`. `g_counter` (int, file-static) and `g_hStatic` (HWND, file-static) — Task 3 reads/writes both directly. `ID_INCREMENT` (1001) and `ID_DECREMENT` (1002) — control IDs, not reused elsewhere.

- [ ] **Step 1: Replace `main.c` to add the static display and two buttons, with click handling**

```c
#include <windows.h>
#include <stdio.h>

#define ID_INCREMENT 1001
#define ID_DECREMENT 1002

static HWND g_hStatic = NULL;
static int g_counter = 0;

static void UpdateCounterText(void) {
    char buf[16];
    sprintf(buf, "%d", g_counter);
    SetWindowTextA(g_hStatic, buf);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFont = CreateFontA(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

        g_hStatic = CreateWindowA("STATIC", "0",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            0, 20, 300, 80, hwnd, NULL, NULL, NULL);
        SendMessageA(g_hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);

        CreateWindowA("BUTTON", "-",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            40, 140, 100, 50, hwnd, (HMENU)ID_DECREMENT, NULL, NULL);

        CreateWindowA("BUTTON", "+",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            160, 140, 100, 50, hwnd, (HMENU)ID_INCREMENT, NULL, NULL);

        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_INCREMENT:
            g_counter++;
            UpdateCounterText();
            break;
        case ID_DECREMENT:
            if (g_counter > 0) g_counter--;
            UpdateCounterText();
            break;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "TapCounterWindowClass";

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(0, CLASS_NAME, "Tap Counter",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 260,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}
```

- [ ] **Step 2: Rebuild and verify it still builds clean**

Run:
```powershell
.\build.bat
```
Expected: `Build succeeded: counter.exe`, no compiler warnings/errors.

- [ ] **Step 3: Launch and verify the process starts without crashing**

Run:
```powershell
$p = Start-Process -FilePath .\counter.exe -PassThru
Start-Sleep -Seconds 1
Get-Process -Id $p.Id -ErrorAction SilentlyContinue
Stop-Process -Id $p.Id -Force
```
Expected: same as Task 1 Step 6 — process is running before being stopped.

- [ ] **Step 4: Commit**

```bash
git add main.c
git commit -m "Add counter display and +/- buttons with floor-at-zero logic"
```

---

### Task 3: Window-click increment, final polish, and handoff for manual verification

**Files:**
- Modify: `main.c:23` (insert a `WM_LBUTTONDOWN` case into the `WndProc` switch, immediately before the `WM_DESTROY` case)
- Modify: `README.md` (already complete from Task 1 — no changes needed unless behavior differs, which it doesn't)

**Interfaces:**
- Consumes: `UpdateCounterText()`, `g_counter` from Task 2 — no new interfaces produced (this is the final task).

- [ ] **Step 1: Insert the `WM_LBUTTONDOWN` case into `WndProc`**

In `main.c`, inside the `switch (msg)` block of `WndProc`, add this case directly before `case WM_DESTROY:`:

```c
    case WM_LBUTTONDOWN:
        g_counter++;
        UpdateCounterText();
        return 0;
```

The `WM_COMMAND` case ends with `return 0;` right above this insertion point — the full switch block should now read `WM_CREATE`, `WM_COMMAND`, `WM_LBUTTONDOWN`, `WM_DESTROY` in that order. This does not double-count button clicks: `BUTTON`-class children handle their own mouse-down internally and notify the parent via `WM_COMMAND`/`BN_CLICKED`, not by bubbling `WM_LBUTTONDOWN` up to the parent's `WndProc`. The `STATIC` counter display has no `SS_NOTIFY` style, so Win32 treats it as click-through (`HTTRANSPARENT`) for hit-testing, meaning a click on the number itself also reaches this handler — consistent with "click anywhere in the window."

- [ ] **Step 2: Rebuild and verify**

Run:
```powershell
.\build.bat
Get-Item .\counter.exe | Select-Object Name, Length
```
Expected: `Build succeeded: counter.exe`; `Length` in the tens-of-KB range (roughly 30,000–80,000 bytes).

- [ ] **Step 3: Launch and verify the process starts without crashing**

Run:
```powershell
$p = Start-Process -FilePath .\counter.exe -PassThru
Start-Sleep -Seconds 1
Get-Process -Id $p.Id -ErrorAction SilentlyContinue
Stop-Process -Id $p.Id -Force
```
Expected: same as prior tasks — process is running before being stopped.

- [ ] **Step 4: Commit**

```bash
git add main.c
git commit -m "Increment counter on any window click"
```

- [ ] **Step 5: Hand off to the user for interactive verification**

Report to the user that the build is complete, and ask them to manually run through the spec's testing checklist (from `docs/superpowers/specs/2026-07-02-tap-click-counter-design.md`):

1. Launch `counter.exe`: window appears with `0` displayed, `+` and `−` buttons visible.
2. Click `+` three times: display reads `3`.
3. Click `−` until it reaches `0`, then click `−` twice more: display stays at `0`.
4. Click on the window background (not on a button): display increments by 1 each time.
5. Click a button once: confirm the count changes by exactly 1, not 2.
6. Resize and close the window: behaves normally, no crash.

---

## Self-Review Notes

- **Spec coverage:** counter start-at-0 (Task 1 `g_counter = 0` static init), `+`/`−` buttons with floor at 0 (Task 2), window-click increment without double-counting buttons (Task 3), small dependency-free exe via `-mwindows -O2 -s` with no comctl32 (Task 1 `build.bat`, verified by size check in Task 3), README/build script/gitignore (Task 1). All spec sections are covered.
- **Placeholder scan:** no TBD/TODO; every step has complete, runnable code or exact commands with expected output.
- **Type consistency:** `g_counter` (int) and `g_hStatic` (HWND) are declared once in Task 2 and referenced identically (same names, same types) in Task 3; `UpdateCounterText(void)` signature is identical everywhere it's used; `ID_INCREMENT`/`ID_DECREMENT` values (1001/1002) are consistent across Tasks 2 and 3.
