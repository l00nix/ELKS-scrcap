# Validation for 0.5.0

Validated on 21 November 2026 against upstream ELKS
`df2a61c5d72415ff4825aab33be427de9306c230`, using its matching freshly built libc
and IA16 GCC 6.3.0. The build explicitly targets i8086.

| Check | Result |
| --- | --- |
| Host fixtures, CLI, output-failure and hotkey-model tests | PASS |
| Native 16-bit build and static ISA / near-memory audit | PASS |
| Native fixture tests under elksemu | PASS |
| Native port-I/O routine on QEMU VGA, modes 10h and 12h | PASS |
| Mocked VGA and original-EGA register access / restoration | PASS |
| Physical Amstrad PPC generic text capture | PASS for capture, transfer and decoded output |
| Physical PPC display appearance after capture | Pending user inspection; user was away |
| Physical EGA / VGA cards | Not yet tested |
| Staged install and 8.3 destination checks | PASS |

The release binary is 14,400 bytes. Its near-memory budget is 54,656 bytes including
2 KiB heap and 2 KiB stack, leaving 10,864 bytes before arguments/environment.
The 38,400-byte raster is split into two blocks below the IA16 per-object limit.
This release uses more resident memory than the old 640×200-only version.

## Automated tests

```sh
make test
# After a native build; QEMU and NASM required for this additional check:
python3 tests/test_video.py build/scrcap.dis
# On a host with the ELKS execution harness:
python3 tests/test_capture.py /path/to/elksemu build/scrcap
python3 tests/test_modes.py /path/to/elksemu build/scrcap
```

Tests cover pixel-exact BMP/PBM headers and raster output, BMP row padding and tail
bits, CGA/HP polarity, forced/automatic backend selection, HP fonts/pages/attributes,
generic colour/MDA text, 40/80 columns, 25/43/50/60 rows, 8/14/16-scanline supplied
fonts, reverse/hidden/underline cells, fallback selection, EGA/VGA planes and pages,
invalid geometry/modes, short reads, output failures and partial-file cleanup,
no overwrite/symlinks, CLI errors, delay and repeated SIGUSR1 capture.

The port mock verifies GC index/read-map/mode restoration, prior interrupt-enable
state, bounded reads and zero reads from original EGA write-only registers.
The QEMU harness extracts the exact compiled `pc_plane_row` routine from the audited
disassembly and executes it against emulated VGA memory/registers. Both the VGA
save/restore path and caller-supplied EGA-state path return correct pixels and
restore registers in modes 10h/12h. QEMU VGA is not a physical EGA card, and this
boot harness does not claim a complete ELKS graphics-application integration test.

The elksemu test copy uses the isolated LP64 `select` timeval fix supplied by
Cmdr's `tests/prepare_emulator.py`. That changes the host harness, not the target
application or kernel. Memory fixtures contain synthetic glyphs, not HP ROM dumps.
Planar fixtures consist of a 1 MiB physical-memory image followed by four 64 KiB
plane banks; normal text/CGA fixtures need only the physical-memory image.

## PPC hardware check

The PPC ran ELKS 0.9.2 (kernel build 15 November 2026). The native release candidate
was uploaded into `/root/sc05`, downloaded again and compared byte-for-byte.
The `pc` backend captured the existing login console at 640×200. A subsequent
text test-page attempt overlapped Cmdr testing from another active chat: the saved
screen contained both applications' output. Console writes were stopped and that
mixed capture is excluded from the README gallery. Its BMP and PBM files still
had identical decoded pixels. The font source was PC ROM lower glyphs with
fallback upper glyphs. Network commands remained responsive after capture.

The test did not switch display modes, replace system binaries or touch the
bootloader. The local display still needs visual confirmation. Existing gallery images retain
their [historical HP provenance](images/README.md).

## Remaining acceptance

- Real EGA/VGA cards, including known-register-profile original EGA capture.
- Physical 43/50-line text and supplied 14/16-scanline custom fonts.
- Graphics applications with their own register/latch usage; capture during idle
  drawing only. Read latches are not restored and concurrent drawing may tear.
- Optional HP hotkey integration against a compatible patched kernel.

External ELKS build and EPT recipe interfaces retain their 0.2 install paths.
Neither a recipe nor this GitHub release makes the package available on EPT's
default server; upstream/server acceptance is separate.
