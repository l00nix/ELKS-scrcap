# External build and EPT integration

Integration submissions (pending maintainer acceptance):

- [ELKS external build PR #2858](https://github.com/ghaerr/elks/pull/2858).
- [EPT recipe PR #12](https://github.com/sammothxc/ept/pull/12).
- Recipes are also committed to [l00nix/ept](https://github.com/l00nix/ept/tree/main/recipes).

The [companion patch](elks-apps.patch) adds both applications to `buildext.sh` and
`elkscmd/ExtApplications`. It was tested against ELKS `df2a61c5d72415ff4825aab33be427de9306c230`.
Apply from a clean ELKS checkout with `git am /path/to/elks-apps.patch`, then run
`./buildext.sh cmdr scrcap`. Do not apply again once the upstream change is present.

From a configured ELKS tree, build this checkout with:

```sh
ELKS=/path/to/elks sh build.sh
```

Output: `build/scrcap`. The script targets i8086, retains ELF/map/disassembly,
and runs the ISA/near-memory audit. The compiler and matching ELKS libc must
already be installed. `CROSS` or `CROSSDIR` selects the toolchain prefix; a
compiler on PATH is also supported. No target kernel is modified.

For ghaerr/elks, register `scrcap` in `buildext.sh`, install the binary/docs using `elkscmd/ExtApplications`.
The app source is cloned into `extapps/scrcap`. No floppy-size tags are added; HD images install built external apps.
For custom images use `CONFIG_APP_TAGS=":scrcap|"` (combine tags with `|` and retain the final `|` for ELKS Make.install).

For EPT, copy `scrcap.pkg` into the server recipe directory and rebuild the index.
Tagged releases provide stable packages; the branch can provide nightlies.
Once a configured EPT repository publishes it, install with `ept install scrcap`.
A GitHub release alone does not add it to EPT's default server.

All installed path components are 8.3-compatible: `/bin/scrcap`,
`/lib/scrcap/copying`, `/lib/scrcap/usage.txt`. Host build filenames are not
installed on ELKS. No compatibility alias using the old executable name is installed.
