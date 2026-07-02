# Always-On-Top Pin Toggle Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a pin button to Tap Click Counter that toggles the window between normal and always-on-top, using the real Windows pin icon.

**Architecture:** Extends the existing single `main.c`. One new `BUTTON` child control (`ID_PIN`) created via the Unicode Win32 APIs (`CreateWindowW`/`SetWindowTextW`) so it can display "Segoe MDL2 Assets" pin glyphs; everything else in the file stays on the ANSI (`...A`) APIs it already uses.

**Tech Stack:** C, Win32 API, mingw-w64 `gcc` — same as the base app. No new dependencies.

## Global Constraints

- No .NET, no Electron, no third-party libraries — Win32 API only.
- Build stays `gcc main.c -o counter.exe -mwindows -O2 -s` — no new build flags, no comctl32.
- Pin state always starts unpinned (`FALSE`) on launch — no persistence, no file I/O.
- Icon font is "Segoe MDL2 Assets" only — no font-detection or fallback logic.
- Pin glyphs: numeric codepoint `0xE718` (outline, off) and `0xE840` (filled, on). Written as numeric `wchar_t` array literals in code (NOT `\u` string escapes — see implementation note below).
- Clicking the pin button must not increment the tap counter (no double-counting with the `WM_LBUTTONDOWN` background-click handler).
- Existing counter/`+`/`−` behavior (floor at 0, no persistence, background-click increment) must be unaffected.

**Implementation note on the glyph literals:** write the two glyphs as
explicit numeric `wchar_t` arrays —
`static const WCHAR PIN_GLYPH_OFF[] = { 0xE718, 0x0000 };` and
`static const WCHAR PIN_GLYPH_ON[] = { 0xE840, 0x0000 };` — NOT as
`L""`-style string escapes. This is because `\u` escapes and raw
Private-Use-Area Unicode characters are invisible and have proven
unreliable when copy-pasted through chat/markdown tooling (they can
silently vanish or get mangled). Numeric array literals use only plain
ASCII hex digits, so they survive any text pipeline intact and are
exactly as valid in C — `PIN_GLYPH_OFF` and `PIN_GLYPH_ON` are ordinary
null-terminated `const WCHAR*`-compatible arrays usable anywhere a
`LPCWSTR` is expected.

---

### Task 1: Pin toggle button

**Files:**
- Modify: `main.c` (full replacement shown below)

**Interfaces:**
- Consumes: existing `g_counter`, `g_hStatic`, `UpdateCounterText()`, `ID_INCREMENT`/`ID_DECREMENT` from the current `main.c`.
- Produces: `ID_PIN` (1003), `g_hPin` (HWND), `g_pinned` (BOOL), `PIN_GLYPH_OFF`/`PIN_GLYPH_ON` (const WCHAR arrays) — no later tasks in this plan consume them; this is the only task.

- [ ] **Step 1: Replace `main.c` with the version below**

```c
#include <windows.h>
#include <stdio.h>

#define ID_INCREMENT 1001
#define ID_DECREMENT 1002
#define ID_PIN 1003

static const WCHAR PIN_GLYPH_OFF[] = { 0xE718, 0x0000 };
static const WCHAR PIN_GLYPH_ON[]  = { 0xE840, 0x0000 };

static HWND g_hStatic = NULL;
static HWND g_hPin = NULL;
static int g_counter = 0;
static BOOL g_pinned = FALSE;

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
            0, 50, 300, 80, hwnd, NULL, NULL, NULL);
        SendMessageA(g_hStatic, WM_SETFONT, (WPARAM)hFont, TRUE);

        CreateWindowA("BUTTON", "-",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            40, 140, 100, 50, hwnd, (HMENU)ID_DECREMENT, NULL, NULL);

        CreateWindowA("BUTTON", "+",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            160, 140, 100, 50, hwnd, (HMENU)ID_INCREMENT, NULL, NULL);

        HFONT hPinFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe MDL2 Assets");

        g_hPin = CreateWindowW(L"BUTTON", PIN_GLYPH_OFF,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            210, 10, 90, 30, hwnd, (HMENU)ID_PIN, NULL, NULL);
        SendMessageW(g_hPin, WM_SETFONT, (WPARAM)hPinFont, TRUE);

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
        case ID_PIN:
            g_pinned = !g_pinned;
            SetWindowPos(hwnd, g_pinned ? HWND_TOPMOST : HWND_NOTOPMOST,
                0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetWindowTextW(g_hPin, g_pinned ? PIN_GLYPH_ON : PIN_GLYPH_OFF);
            break;
        }
        return 0;
    case WM_LBUTTONDOWN:
        g_counter++;
        UpdateCounterText();
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

Note on the structural changes from the current file: (1) two new
top-level `static const WCHAR` array declarations (`PIN_GLYPH_OFF`,
`PIN_GLYPH_ON`) above the existing globals; (2) `g_hStatic`'s
`CreateWindowA` y-coordinate changed from `20` to `50` to make room for
the pin button above it; (3) everything from `HFONT hPinFont = ...`
through `SendMessageW(g_hPin, ...)` is new, inserted after the `+` button
creation and before the `WM_CREATE` case's `return 0;`. The `ID_PIN` case
in `WM_COMMAND` is new; `WM_LBUTTONDOWN` and `WM_DESTROY` are unchanged.

- [ ] **Step 2: Rebuild and verify it compiles clean**

Run:
```powershell
.\build.bat
```
Expected: `Build succeeded: counter.exe`, no compiler warnings or errors.

- [ ] **Step 3: Launch and verify the process starts without crashing**

Run:
```powershell
$p = Start-Process -FilePath .\counter.exe -PassThru
Start-Sleep -Seconds 1
Get-Process -Id $p.Id -ErrorAction SilentlyContinue
Stop-Process -Id $p.Id -Force
```
Expected: `Get-Process` returns a running `counter` process entry before
it's stopped — confirms the window (including the new pin button) opened
without crashing on startup.

- [ ] **Step 4: Commit**

```bash
git add main.c
git commit -m "Add always-on-top pin toggle with Windows pin icon"
```

- [ ] **Step 5: Hand off to the user for interactive verification**

Report to the user that the build is complete, and ask them to manually
run through the design doc's testing checklist (from
`docs/superpowers/specs/2026-07-02-always-on-top-pin-design.md`):

1. Launch `counter.exe`: pin button shows the outline pin glyph, window
   behaves normally (not topmost).
2. Click the pin button: icon switches to the filled glyph; window stays
   on top of other windows even when they lose focus.
3. Click the pin button again: icon switches back to outline; window no
   longer stays on top.
4. Click the pin button once: confirm the tap counter does NOT change.
5. Counter display and `+`/`−` buttons still work exactly as before, with
   no visual overlap from the pin button or the shifted counter position.

---

## Self-Review Notes

- **Spec coverage:** icon rendering via Segoe MDL2 Assets glyphs `0xE718`/`0xE840` (Step 1), no persistence (`g_pinned` is a plain static, no file I/O anywhere in the diff), no double-counting with the tap counter (`ID_PIN` is handled entirely inside `WM_COMMAND`, same mechanism that already isolates `ID_INCREMENT`/`ID_DECREMENT` from `WM_LBUTTONDOWN`), layout shift (`g_hStatic` y: 20→50) — all present. Testing checklist mirrors the spec's Testing section exactly (Step 5).
- **Placeholder scan:** no TBD/TODO; full file content given, not a diff fragment, so there's no ambiguity about where changes land.
- **Type consistency:** `g_pinned` (BOOL), `g_hPin` (HWND), `ID_PIN` (1003), `PIN_GLYPH_OFF`/`PIN_GLYPH_ON` (const WCHAR arrays, used as LPCWSTR) are declared once and used consistently within the single task; no cross-task interface to check since this plan has only one task.
