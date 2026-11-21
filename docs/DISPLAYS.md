# Display support and portability

ELKS-scrcap is intended for 8086/8088 PCs running ELKS, across multiple display
adapters and resolutions. HP 200LX support is one backend, not the boundary of
the project. CPU compatibility alone does not make video memory layouts, fonts,
registers or keyboard interfaces interchangeable.

## Current implementation versus GEMCAP 4.0

The supplied GEMCAP 4.0 manual (Natural Software, July 1990), Introduction,
Text Mode Screens and Graphic Mode Screens sections, describes the following.
These are documentation comparisons, not tests of GEMCAP or claims of implemented
scrcap features.

| Display / mode | GEMCAP 4.0 manual | ELKS-scrcap 0.2.0 |
| --- | --- | --- |
| CGA mode 06h, 640×200 | Supported, monochrome | Implemented; generic backend fixture-tested |
| EGA mode 10h, 640×350, 16 colours | Supported; converted to monochrome | Planned; not implemented |
| VGA mode 12h, 640×480, 16 colours | Supported; converted to monochrome | Planned; not implemented |
| MDA / CGA / Hercules text | Rasterized using a built-in 8×8 font | Planned generic text backend; not implemented |
| EGA / VGA text | Uses the adapter's loaded font; includes 43- and 50-line screens | Planned variable-geometry text backend; not implemented |
| Hercules graphics | Explicitly unsupported | Not implemented; separate future backend |
| HP 200LX graphics / text | Not specifically described in these manual sections | Implemented: mode 6 and HP 80×25 text modes 2/3, exported at 640×200 |

GEMCAP's EGA/VGA output is black and white, despite accepting colour source modes.
It does not preserve those colours in its IMG files. BMP and PBM can encode the
monochrome result losslessly at different dimensions; 640×200 is not a restriction
of either file format. Colour-to-monochrome conversion itself discards colour.

## Why the current code is fixed at 640×200

`src/capture.h` defines a 16,000-byte pixel buffer. `src/capture.c` assumes
80 bytes per scanline, 200 scanlines, and those dimensions in both output headers.
Its generic backend decodes CGA's two video-memory banks. Its text path currently
uses HP firmware fonts. Merely increasing the dimensions would not decode EGA/VGA
planes, MDA memory, active fonts or different text geometries correctly.

## Implementation sequence

1. **Variable geometry and bounded memory.** Introduce a capture descriptor for
   width, height, row stride, source mode and monochrome output. Generate BMP/PBM
   headers and BMP row padding from the descriptor. Keep the 8086 near-memory
   budget explicit; use bounded row/plane buffers or deliberate allocation limits
   rather than assuming every framebuffer fits in a single 64 KiB segment.
2. **Generic PC text capture.** Support B800 colour text and B000 monochrome text,
   active pages, attributes and supported row/column counts. Use a redistributable
   fallback font and label substituted-font captures. Adapter-loaded fonts need a
   separate reader to reproduce customized EGA/VGA text accurately. Retain HP's
   font path as a separate backend.
3. **EGA and VGA graphics.** Add mode 10h and 12h readers with an explicit
   monochrome conversion policy. Access to plane-selection and font registers
   needs a supported ELKS mechanism, coordination with the display owner and
   restoration of all altered register state. A new backend must not leave the
   running application or display changed after capture or error.
4. **Additional adapters.** Evaluate Hercules graphics and other ELKS-supported
   hardware separately. PC-98 and non-IBM-compatible video systems require their
   own backends; they cannot inherit IBM CGA/EGA/VGA support just from sharing
   an 8086-family CPU.
5. **Portable capture trigger.** Ordinary, delayed and signal-triggered captures
   remain independent of the optional HP hotkey patch. A general ELKS keyboard
   shortcut would need a separately reviewed kernel/console integration.

## Acceptance for each added mode

- Synthetic fixtures verify decoded pixels, dimensions, stride/padding, active
  pages, attributes and explicit rejection of unsupported modes.
- Output failures, interrupted capture and any changed hardware state are handled.
- Native 8086 builds pass ISA and memory-budget checks; emulator tests supplement
  real-hardware acceptance rather than replace it.
- Hardware tests identify the machine, adapter, mode, font and capture method.
  The Amstrad PPC is a useful next test target once its active display mode and
  ELKS display-driver path are confirmed.
- Support is advertised per verified adapter/mode, not as blanket support for
  every PC on which ELKS can boot.

These items are planned work. This documentation update adds no new capture modes
and does not change the published 0.2.0 executable.

## Reference

Reviewed the user-supplied `gemcap.zip`, specifically `GEMCAP.DOC`, identifying
itself as GEMCAP 4.0 by Natural Software. The original package/manual is not
redistributed in this repository. A public historical reference is the
[GEMCAP listing in the preserved S.U.P.E.R. archive](https://www.ledudu.com/HPLX/super7.html),
under “Screen Capture.” This is an independent implementation inspired by its
workflow, not a source-code port.

GEMCAP.DOC SHA-256: `60818e3694df39e4e3bf16da577f81e73450b2fbd5023a96723f06852869f600`.
