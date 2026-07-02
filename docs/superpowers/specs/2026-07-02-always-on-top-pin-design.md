# Always-On-Top Pin Toggle — Design

## Purpose

Add a pin toggle to Tap Click Counter so the window can be kept above all
other windows, the way Windows 11 Calculator's "Keep on top" pin works.

## Behavior

- A small button in the top-right of the window toggles the window between
  normal and topmost (always-on-top).
- The button's icon is the actual Windows pin glyph (outline when off,
  filled when on) — not text, not an emoji.
- Starts unpinned every launch. No persistence, no file I/O — consistent
  with the counter's own no-persistence design.
- Clicking the pin button does not increment the tap counter (same
  mechanism that already keeps the `+`/`−` buttons from double-counting
  against the window-background click handler).

## Icon Rendering

Use the **"Segoe MDL2 Assets"** icon font, which ships on both Windows 10
(1809+) and Windows 11 and contains the pin glyphs Windows itself uses in
Calculator, File Explorer, etc. No font-detection/fallback logic is
needed — "Segoe MDL2 Assets" alone covers the supported OS range.

- Codepoint `U+E718` — outline pin (off state)
- Codepoint `U+E840` — filled pin (on state)

These glyphs are outside ASCII, so the pin button is created and updated
via the Unicode Win32 APIs (`CreateWindowW`, `SetWindowTextW`,
`CreateFontW`) rather than the ANSI (`...A`) APIs the rest of the app
uses. Only this one control needs Unicode; nothing else in the app
changes API family.

## Components

Extends the existing single `main.c`:

- New control ID `ID_PIN` (1003 — 1001/1002 are already taken by
  `ID_INCREMENT`/`ID_DECREMENT`).
- New file-static `HWND g_hPin` and `BOOL g_pinned = FALSE`.
- `WM_CREATE`: after creating the existing controls, create the pin
  button via `CreateWindowW`, passing the outline pin glyph (codepoint
  `U+E718`) as its initial text, at position `210, 10, 90, 30` with
  `WS_CHILD | WS_VISIBLE` and control ID `ID_PIN`. Apply a `CreateFontW`
  with face name `L"Segoe MDL2 Assets"` sized to fit the button via
  `WM_SETFONT` (same pattern already used for the counter's font).
- `WM_COMMAND` gains a case for `ID_PIN`: flip `g_pinned`, call
  `SetWindowPos(hwnd, g_pinned ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0,
  0, SWP_NOMOVE | SWP_NOSIZE)`, and `SetWindowTextW` the pin button to
  the filled glyph (`U+E840`) when pinned, or back to the outline glyph
  (`U+E718`) when unpinned.
- Existing counter display (`g_hStatic`) shifts from y=20 to y=50 to make
  room for the pin button above it. The `+`/`−` buttons are unchanged —
  the existing gap between the counter and the buttons absorbs the shift.

## Data Flow

Pin button click → `WM_COMMAND` (`ID_PIN`) → flip `g_pinned` → `SetWindowPos`
changes the OS-level z-order rule for the window → `SetWindowTextW` updates
the glyph shown. No interaction with `g_counter` or `UpdateCounterText`.

## Testing

Same approach as the rest of the app — no automated test framework (still
a small GUI program). Manual verification after building:

1. Launch: pin button shows the outline pin glyph, window behaves normally
   (not topmost).
2. Click the pin button: icon switches to the filled glyph; window stays
   on top of other windows even when they're focused.
3. Click the pin button again: icon switches back to outline; window no
   longer stays on top.
4. Click the pin button once: confirm the tap counter does NOT change
   (no double-counting with the window-background click handler).
5. Counter display and `+`/`−` buttons still work exactly as before, with
   no visual overlap from the pin button or the shifted counter position.

## Out of Scope

- Persisting the pinned state across runs.
- Keyboard shortcut or system-menu entry for the toggle.
- Any icon font other than "Segoe MDL2 Assets" (no runtime font-detection
  or fallback).
