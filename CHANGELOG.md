# Changes

## 0.5.0

- Variable BMP/PBM dimensions and bounded raster storage up to 640×480.
- Generic B800/B000 text capture, active pages, 40/80 columns, 43/50-line modes,
  reverse/hidden cells and MDA underline; configurable rows, height and raw font.
- Redistributable ELKS fallback font; font source reported for each capture.
- EGA 640×350 and VGA 640×480 monochrome planar capture, plus VGA mode 10h.
- Bounded native port-I/O with register/interrupt restoration; original EGA
  requires a known register profile because its GC registers are write-only.
- Physical PPC text capture, expanded host/native fixtures and QEMU VGA checks.
- Existing HP, delayed capture, SIGUSR1 and optional HP hotkey paths retained.
- EGA/VGA font RAM, colour output and physical EGA/VGA validation remain outside
  this release; see docs/DISPLAYS.md for exact limits.

## 0.2.0

- Standalone scrcap repository and 8.3-compatible installation paths.
- External ELKS build entry point and EPT package recipe.
- Generic IBM-compatible CGA mode 6 capture, retaining the HP backend.
