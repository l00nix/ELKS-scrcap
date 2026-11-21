#!/usr/bin/env python3
"""Independent golden pixels for generic text and planar graphics, host or elksemu."""
import os, struct, subprocess, sys, tempfile
from pathlib import Path
cmd = [str(Path(p).resolve()) for p in sys.argv[1:]]
def run(args, code=0):
    p = subprocess.run(cmd+args, capture_output=True, timeout=20)
    assert p.returncode == code, (args,p.returncode,p.stdout,p.stderr)
    return p

def decode(path,w,h):
    b=Path(path).read_bytes();stride=(w+7)//8;pad=(stride+3)&~3
    assert struct.unpack_from('<IiiHHII',b,14)==(40,w,h,1,1,0,pad*h)
    assert len(b)==struct.unpack_from('<I',b,2)[0]==62+pad*h
    return b''.join(b[62+y*pad:62+y*pad+stride] for y in range(h-1,-1,-1))

def memory(mode,cols=80,rows=25,height=8,page=0):
    b=bytearray(0x140000)
    b[0x449]=mode;struct.pack_into('<H',b,0x44a,cols)
    struct.pack_into('<H',b,0x44e,page);b[0x484]=rows-1
    struct.pack_into('<H',b,0x485,height)
    return b

with tempfile.TemporaryDirectory(prefix='scrcap-modes-') as d:
    os.chdir(d)
    # Every glyph and scanline differs; checks 8/14/16 fonts without trusting renderer.
    for mode,cols,rows,height in [(0,40,25,8),(3,80,25,8),(7,80,25,14),(3,80,25,16),(3,80,43,8),(3,80,50,8),(3,80,60,8)]:
        font=bytes((ch*17+y*31)&255 for ch in range(256) for y in range(height))
        Path('font.bin').write_bytes(font)
        page=4096;b=memory(mode,cols,rows,height,page)
        base=(0xb0000 if mode==7 else 0xb8000)+page
        for row in range(rows):
            for col in range(cols):
                ch=(row*cols+col)%256;attr=[7,0x70,0,0x87][col%4]
                b[base+2*(row*cols+col):base+2*(row*cols+col)+2]=bytes([ch,attr])
        Path('mem.img').write_bytes(b)
        stem=f't{mode}-{cols}-{rows}-{height}'
        run(['--backend','pc','--image','mem.img','--font','font.bin',stem+'.bmp'])
        got=decode(stem+'.bmp',cols*8,rows*height)
        want=bytearray()
        for row in range(rows):
            for y in range(height):
                for col in range(cols):
                    v=font[((row*cols+col)%256)*height+y]
                    want.append(v if col%4 in (0,3) else v^255 if col%4==1 else 0)
        assert got==want,stem
        run(['--backend','pc','--image','mem.img','--font','font.bin','-f','pbm',stem+'.pbm'])
        assert Path(stem+'.pbm').read_bytes()==f'P4\n{cols*8} {rows*height}\n'.encode()+want
    # Override legacy BDA values, use MDA B000 cells and underline.
    b=memory(7);b[0x484]=0;b[0xb0000:0xb0002]=bytes([65,1]);Path('mem.img').write_bytes(b)
    Path('font.bin').write_bytes(bytes(2048))
    run(['--image','mem.img','--font','font.bin','--rows','25','--height','8','mda.bmp'])
    pixels=decode('mda.bmp',640,200);assert pixels[6*80]==255 and sum(pixels)==255
    # Default fallback is usable even with unreadable/empty ROM; label substitution.
    p=run(['--image','mem.img','fallback.bmp']);assert b'fallback' in p.stdout
    # Mode 10h page 1 and mode 12h page 0. Other planes carry conflicting values.
    for mode,height,page,backend in [(0x10,350,32768,'ega'),(0x10,350,0,'vga'),(0x12,480,0,'auto')]:
        b=memory(mode,page=page)
        b[0x100000:]=bytes([0x55])*0x40000
        want=bytes((y*29+x*13)&255 for y in range(height) for x in range(80))
        b[0x120000+page:0x120000+page+len(want)]=bytes(v^255 for v in want)
        Path('mem.img').write_bytes(b);stem=f'g{mode}-{page}-{backend}'
        run(['--backend',backend,'--image','mem.img',stem+'.bmp'])
        assert decode(stem+'.bmp',640,height)==want
        run(['--backend',backend,'--image','mem.img','-i','-f','pbm',stem+'.pbm'])
        assert Path(stem+'.pbm').read_bytes()==f'P4\n640 {height}\n'.encode()+bytes(v^255 for v in want)
    # Invalid geometries and ambiguous or unsupported hardware layouts must fail.
    for b,args in [(memory(3,cols=132),[]),(memory(3,rows=50,height=16),[]),(memory(3,page=65534),[]),(memory(0x10),[]),(memory(0x12),['--backend','ega']),(memory(0x13),[]),(memory(0x12,page=32768),[])]:
        Path('bad.img').write_bytes(b)
        run(['--image','bad.img']+args+['bad.bmp'],1);assert not Path('bad.bmp').exists()
    Path('bad.img').write_bytes(memory(0x12)[:0x120100])
    run(['--image','bad.img','bad.bmp'],1)
    Path('mem.img').write_bytes(memory(3));Path('font.bin').write_bytes(bytes(2047))
    run(['--image','mem.img','--font','font.bin','bad.bmp'],1)
    for args in [['--rows','61'],['--rows','-1'],['--height','9'],['--ega-state','9,0,0'],['--ega-state','0,4,0'],['--ega-state','0,0,256'],['--ega-state','0,0'],['--ega-state','0,0,0,0']]:run(args,2)
    assert b'0.5.0' in run(['--version']).stdout
print('PASS variable geometry; PC/CGA/MDA text, 40/80 cols, 25/43/50/60 rows, 8/14/16 fonts; EGA/VGA golden pixels/pages; bounds and invalid modes')
