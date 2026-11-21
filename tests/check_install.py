from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
s=(root/'integration/scrcap.pkg').read_text()
paths=re.findall(r':(/[^\s"\n]+)',s)
assert paths
for path in paths:
 for component in path.strip('/').split('/'):
  assert re.fullmatch(r'[A-Za-z0-9_+-]{1,8}(?:\.[A-Za-z0-9_]{1,3})?',component),path
print('PASS installed paths use 8.3 names')
