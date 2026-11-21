#!/usr/bin/env python3
"""Exercise native port I/O on QEMU VGA, not just mock APIs or memory fixtures.
Requires an audited build/scrcap.dis, nasm and qemu-system-i386.
The original-EGA access path runs on VGA emulation; this is NOT an EGA card test.
"""
import re,subprocess,tempfile,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
dis=Path(sys.argv[1] if len(sys.argv)>1 else root/'build/scrcap.dis').read_text()
body=dis.split('<pc_plane_row>:',1)[1].split('\n\n',1)[0]
assert not re.search(r'\bcall\b',body), 'harness requires self-contained function'
code=b''.join(bytes.fromhex(m[1]) for m in re.finditer(r'^\s+[0-9a-f]+:\s+((?:[0-9a-f]{2} )+)',body,re.M))
assert 100<len(code)<1024
with tempfile.TemporaryDirectory(prefix='scrcap-vga-') as d:
 p=Path(d);(p/'pcfn.bin').write_bytes(code)
 subprocess.run(['nasm','-f','bin',str(root/'tests/video.asm'),'-o','boot.img'],cwd=p,check=True)
 # Give BIOS a conventional 1.44MB floppy, though only five sectors are used.
 with (p/'boot.img').open('ab') as f:f.truncate(1474560)
 r=subprocess.run(['qemu-system-i386','-display','none','-serial','none','-monitor','none','-no-reboot','-drive',f'file={p}/boot.img,format=raw,if=floppy','-debugcon',f'file:{p}/debug.log','-device','isa-debug-exit,iobase=0xf4,iosize=4'],capture_output=True,timeout=30)
 result=(p/'debug.log').read_text()
 assert r.returncode==33 and result.startswith('PASS'),(r.returncode,result,r.stderr)
 print(result.strip())
