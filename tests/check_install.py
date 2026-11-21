from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
s=(root/'integration/scrcap.pkg').read_text()
# Only FILES destinations are installed; the GIT URL is a host-side location.
files=re.search(r'^FILES="([^"]*)"',s,re.M|re.S)
assert files, 'missing FILES recipe mapping'
paths=[pair.split(':',1)[1] for pair in files.group(1).split()]
assert all(path.startswith('/') for path in paths)
assert paths
for path in paths:
 for component in path.strip('/').split('/'):
  assert re.fullmatch(r'[A-Za-z0-9_+-]{1,8}(?:\.[A-Za-z0-9_]{1,3})?',component),path
print('PASS installed paths use 8.3 names')
