# scrcap

[Download 0.2.0](https://github.com/l00nix/scrcap/releases/tag/v0.2.0) · [Integration status](integration/README.md)

Lossless monochrome screenshots for [ELKS](https://github.com/ghaerr/elks).
The executable is **`scrcap`** (formerly `elxcap` / ELXcap).
Save a 640x200 screen as 1-bit BMP or binary PBM without changing the display mode.

## Supported displays

| Backend | Support |
| --- | --- |
| `auto` (default) | Detect HP 200LX firmware; otherwise use the CGA backend |
| `cga` | IBM-compatible CGA mode 6, 640x200, page 0 |
| `hp` | HP 200LX mode 6 and 80x25 text modes 2/3 using the machine's ROM font |

The generic CGA path does not require an HP ROM or an HP kernel. It is fixture-tested;
other physical ELKS machines still need validation. Generic PC text, MDA, Hercules,
other VGA/EGA modes and PC-98 are not implemented. Unsupported modes/pages are rejected.
The tool reads `/dev/kmem` (normally as root) and captures the physical console,
not a serial terminal. Hardware cursor, LCD zoom and LCD inversion are not reproduced.
HP ROM data is read on the device and is not distributed with this project.

## Use

```sh
scrcap /root/screen.bmp
scrcap -f pbm /root/screen.pbm
scrcap --backend cga -d 15 /root/screen.bmp
scrcap -q -d 15 >/tmp/cap.log 2>&1 &
```

With no filename, files are numbered `CAP0000.BMP`, `CAP0001.BMP`, and so on.
Existing files are never overwritten. `-i` inverts the exported pixels; `-f pbm`
selects PBM (the suffix alone does not select a format). Both formats preserve the
same pixels; BMP is convenient for desktop viewers.

For repeated captures without a kernel patch:

```sh
scrcap --watch /root >/tmp/cap.log 2>&1 &
# Send SIGUSR1 to that process for each capture; SIGTERM stops it.
```

`--hotkey /root` is an optional **HP-only** Alt+period listener requiring a compatible
kernel patch. The legacy patch is retained in `kernel/` for reference, is not applied
by the build/install scripts, and is not a promise of compatibility with current
upstream kernels. Ordinary, delayed and SIGUSR1 captures need no hotkey patch.
`--image FILE` is for offline test fixtures, not live hardware.

## Build and install

Use a configured [ELKS source tree](https://github.com/ghaerr/elks) with its
IA16 compiler and matching libc installed:

```sh
ELKS=/path/to/elks sh build.sh
make install DESTDIR=/path/to/staging-root
```

The build targets the 8086 and audits the retained ELF, disassembly and near-memory
layout. Output is `build/scrcap`. Install paths are `/bin/scrcap` and
`/lib/scrcap/{copying,usage.txt}`; every installed component fits 8.3 naming.
The staging root must exist on the host; do not point it at an unintended filesystem.

[External build and EPT integration](integration/README.md) includes the package
recipe. Upstream registration and EPT server publication are separate steps;
the recipe alone does not make the package available from the default EPT server.

## Tests and status

```sh
make test
```

Host tests require a C compiler and Python 3. Native builds are additionally checked with `elksemu`; synthetic memory fixtures
are not hardware captures. See [validation](docs/TESTING.md) for this release.

## Origins and license

Developed by Alexander Rau with AI assistance, originally as part of the
[HP 200LX ELX suite](https://github.com/l00nix/HP-200LX-elx-suite).
The standalone source starts from suite commit `a2cc3f3`; historical suite releases
retain their original names. ELKS provides the operating system, libc and toolchain.
Project code is [GPL-2.0-or-later](COPYING). Existing attribution is retained.

Inspired by GEMCAP's hotkey-to-file workflow; no GEMCAP source or binaries are included.
