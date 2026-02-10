# dump_shellcode.py
from pathlib import Path

EXE_PATH = "main.exe"
OUT_PATH = "text.bin"

POINTER_TO_RAW = 0x400
SIZE_OF_RAW = 0x400

data = Path(EXE_PATH).read_bytes()
chunk = data[POINTER_TO_RAW:POINTER_TO_RAW + SIZE_OF_RAW]
Path(OUT_PATH).write_bytes(chunk)

print(f"[+] Wrote {len(chunk)} bytes to {OUT_PATH}")
