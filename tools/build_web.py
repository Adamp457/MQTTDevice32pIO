"""Gzip web/index.htm into include/index_htm.h (the PROGMEM blob handleRoot() serves).

Runs before every PlatformIO build (extra_scripts = pre:tools/build_web.py), and by hand:
    python3 tools/build_web.py
mtime=0 keeps the output byte-stable, so an unchanged page gives no diff.
"""
import gzip
import os

try:
    Import("env")  # noqa: F821 - provided by PlatformIO
    root = env["PROJECT_DIR"]  # noqa: F821
except NameError:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

src = os.path.join(root, "web", "index.htm")
dst = os.path.join(root, "include", "index_htm.h")

with open(src, "rb") as f:
    data = gzip.compress(f.read(), compresslevel=9, mtime=0)

lines = []
for i in range(0, len(data), 12):
    lines.append("  " + ", ".join("0x%02x" % b for b in data[i:i + 12]))
out = "const char index_htm_gz[] PROGMEM = { \n" + ",\n".join(lines) + "\n};\nunsigned int index_htm_gz_len = %d;\n" % len(data)

old = open(dst).read() if os.path.exists(dst) else ""
if out != old:
    with open(dst, "w") as f:
        f.write(out)
    print("build_web: rebuilt include/index_htm.h (%d bytes gzipped)" % len(data))
