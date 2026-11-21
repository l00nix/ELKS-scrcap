# Display support in 0.5

ELKS-scrcap targets IBM-compatible 8086/8088 PCs running ELKS. The HP 200LX is
one backend. Screen geometry now comes from the selected mode and BIOS Data Area
(BDA), with an explicit maximum of 640 pixels wide and 480 scanlines.

| Source | Output | Selection and limits |
| --- | --- | --- |
| CGA mode 06h | 640×200 monochrome | `auto`, `pc` or `cga`; page 0 |
| PC colour text, modes 00h–03h | 320 or 640 pixels wide | 40/80 columns, B800 memory, active BIOS page |
| MDA / Hercules text, mode 07h | 640 pixels wide | 80 columns, B000 memory; includes underline |
| EGA/VGA text | Up to 640×480 | BDA rows and 8/14/16 scanline glyphs; 43/50 rows supported |
| EGA mode 10h, 16 colours | 640×350 monochrome | `--backend ega`, exact register profile required on original EGA |
| VGA modes 10h / 12h, 16 colours | 640×350 / 640×480 monochrome | `--backend vga`; mode 12h also selected by `auto` / `pc` |
| HP 200LX graphics / text | 640×200 monochrome | HP ROM detection or `--backend hp`; mode 6 or 80×25 modes 2/3 |

`auto` checks the HP ROM signature first. `pc`, `cga`, `ega` and `vga` skip HP
identification. Mode 10h alone cannot identify whether the adapter is EGA or VGA,
so an explicit backend is required. Other modes fail without creating a capture.
PC-98, Hercules graphics, CGA four-colour graphics, VGA mode 13h, VESA/SVGA,
nonstandard planar layouts, split screens and hardware panning are not supported.
EGA mode 10h here means the four-plane 16-colour layout, not the 64 KiB EGA
four-colour variation.

## Text geometry and fonts

Text cells are read from the active BIOS page. Rows and glyph height come from
the BDA; legacy absent values default to 25 rows and 8 scanlines. Stale BDA data
can be overridden, for example:

```sh
scrcap --backend pc --rows 25 --height 8 text.bmp
scrcap --backend vga --rows 50 --height 8 text50.bmp
scrcap --backend vga --rows 25 --height 16 --font font16.bin textvga.bmp
```

`--rows` accepts 1–60 and `--height` accepts 8, 14 or 16, provided the result fits
640×480. Every exported character cell is **8 pixels wide**: MDA/VGA's ninth
hardware dot is not reproduced. A 43×8 screen is exported at 640×344; unused
border/scanlines are excluded.

For generic 8-scanline text, scrcap tries the PC ROM lower 128 glyphs and INT 1F
upper glyph table, with an [ELKS-derived fallback](FONT.md). For 14/16-scanline
text it scales the fallback unless given `--font`. The success message reports
the font source. **The adapter's loaded EGA/VGA font RAM is not read in 0.5.**
Use `--font` for exact customized glyphs: an unheadered file containing 256 glyphs,
each with exactly the selected number of bytes, one byte per scanline, MSB left.
HP text retains its own firmware-font path.

Text is rendered in black and white. Foreground/background brightness determines
normal or reverse cells; equal brightness is blank. MDA underline is rendered.
Blink is shown steadily visible; hardware cursor and blink phase are omitted.
Background bit 7 is treated as blink, not bright-background mode. Font substitution
and these rasterization rules mean text output is not necessarily a pixel-identical
photograph of the physical display.

## EGA/VGA graphics and register access

The monochrome conversion follows GEMCAP's documented colour groups: red, magenta,
yellow and white (and their bright variants) become white; black, blue, green and
cyan (and their bright variants) become black. This reads the red/index-bit-2
plane; it is **not luminance conversion or colour preservation**. Custom palettes
can therefore produce results unlike their apparent brightness. `-i` reverses the
exported pixels.

VGA's Graphics Controller registers are readable. scrcap saves the selected GC
index, read-map register and mode register, selects plane 2/read mode 0, reads at
most 80 bytes, and restores all three. Interrupts are disabled only for that bounded
row operation; the prior interrupt state is restored. No system call, file write,
mode switch or video-memory write occurs inside it. This direct real-mode I/O path
is compiled only for IA16 ELKS. Ordinary capture requires `/dev/kmem`, normally root.

**Original EGA has write-only GC registers.** Its live backend therefore refuses
to run without `--ega-state INDEX,READMAP,MODE`, giving the exact current values
left by the display owner (decimal or `0x` hexadecimal). This is an advanced
application-integration option, not a universal EGA preset. Do not guess a profile
or copy one from a different application. The caller must know that the standard
mode-10h addressing layout and those register values are in use. On VGA hardware
running mode 10h, use `--backend vga` instead; it saves actual register values.

Capture should be triggered while drawing is idle. VRAM reads change the adapter's
read latches; 0.5 does not preserve those latches for an application relying on
write-mode-1 operations across a task switch. BIOS mode/page changes during capture
are detected, but concurrent drawing can still tear, and custom CRTC start/offset
changes are not reconstructed. Hardware-specific testing remains necessary.

Reference: [VGA Graphics Controller registers](https://www.osdever.net/FreeVGA/vga/graphreg.htm).

## Memory and output

The maximum raster occupies 38,400 bytes in two 19,200-byte blocks, avoiding the
IA16 compiler's 32,767-byte single-object limit. BMP headers, bottom-up rows,
4-byte row padding and PBM top-down rows are derived from the captured dimensions.
Both formats encode the same monochrome pixels without further loss.

## GEMCAP comparison and provenance

The supplied GEMCAP 4.0 manual (Natural Software, July 1990) describes CGA 640×200,
EGA 640×350, VGA 640×480, generic text and 43/50-line EGA/VGA text. Those graphics
sizes and text geometries are now implemented here. GEMCAP reads the EGA/VGA
adapter font; scrcap 0.5 instead uses the font policy above. Both omit Hercules
graphics. This is an independent implementation, not a source-code port.

The original archive/manual is not redistributed. Public historical reference:
[GEMCAP in the preserved S.U.P.E.R. archive](https://www.ledudu.com/HPLX/super7.html),
“Screen Capture.” GEMCAP.DOC SHA-256:
`60818e3694df39e4e3bf16da577f81e73450b2fbd5023a96723f06852869f600`.

See [validation](TESTING.md) for the distinction between physical hardware,
QEMU VGA and synthetic-fixture results.
