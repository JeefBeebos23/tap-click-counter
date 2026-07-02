# Tap Click Counter — Design

## Purpose

A minimal Windows desktop app: a window displaying a counter, with `+` and `−`
buttons, where clicking anywhere in the window also increments the count.
Priority is a very small, dependency-free `.exe`.

## Tech Stack

Raw Win32 API in C, compiled with mingw-w64 `gcc` (already installed on the
target machine). No external libraries, no runtime dependency (not .NET,
not Electron). This keeps the final binary in the tens-of-KB range and
lets it run on any Windows machine with no installation step.

## Behavior

- Counter starts at `0` on launch. No persistence between runs.
- `+` button: counter + 1.
- `−` button: counter − 1, floored at `0` (never goes negative).
- Clicking anywhere in the window background (i.e., not on a button):
  counter + 1.
- The displayed count updates immediately after every action above.

## Components

Single `main.c`:

- `WinMain` — registers the window class, creates the main window, runs the
  message loop.
- `WndProc` — handles window messages:
  - `WM_CREATE`: creates the child controls — a static text control (count
    display) and two `BUTTON`-class child windows (`+` and `−`), using
    fixed IDs (`ID_INCREMENT`, `ID_DECREMENT`).
  - `WM_COMMAND`: dispatches on control ID — increment or decrement the
    counter, floor at 0, update the static text control's label.
  - `WM_LBUTTONDOWN`: increments the counter and updates the label. Because
    button clicks are delivered as `WM_COMMAND` (via `BN_CLICKED`) rather
    than bubbling up as `WM_LBUTTONDOWN` on the parent, clicking a button
    does not double-count against the window-click handler.
  - `WM_DESTROY`: `PostQuitMessage(0)`.
- A single `int counter` (window-local, e.g. via `GetWindowLongPtr`/
  `SetWindowLongPtr` or a file-static since there's only one window) holds
  state; no persistence layer.

## Data Flow

User input (button click or window click) → `WndProc` message handler →
mutate `counter` in place → `SetWindowText` on the static control to reflect
the new value. No other data flow; no files read or written at runtime.

## Build

- `gcc main.c -o counter.exe -mwindows -O2 -s`
  - `-mwindows`: suppress the console window (GUI subsystem).
  - `-s`: strip symbols to minimize exe size.
  - Static by default with mingw-w64 for the Win32 API surface used here
    (no `comctl32` manifest/theming needed for plain `BUTTON`/`STATIC`
    classes).
- `build.bat` wraps the above command for a one-step rebuild.
- Expected output size: roughly 30–80 KB.

## Repo Layout

```
tap-click-counter/
  main.c
  build.bat
  .gitignore     (ignores *.exe and other build artifacts)
  README.md      (brief build/run instructions)
  docs/superpowers/specs/2026-07-02-tap-click-counter-design.md
```

## Testing

Manual verification after building (no automated test framework — this is a
~150-line GUI program where the payoff of unit tests doesn't clear the
cost; behavior is verified by direct interaction):

1. Build with `build.bat`; confirm `counter.exe` is produced and is small
   (double-digit-to-low-double-digit KB).
2. Launch the exe: window appears with `0` displayed, `+` and `−` buttons
   visible.
3. Click `+` three times: display reads `3`.
4. Click `−` until it reaches `0`, then click `−` twice more: display stays
   at `0` (never negative).
5. Click on the window background (not on a button): display increments by
   1 each time.
6. Click a button once: confirm the count changes by exactly 1 (not 2) —
   verifies the button click isn't double-counted against the
   window-background click handler.
7. Resize and close the window: behaves normally, no crash.

## Out of Scope

- Persistence of the counter value across runs.
- Configurable step size, themes, or window resizing behavior beyond
  default OS behavior.
- Any installer/packaging beyond the single exe.
