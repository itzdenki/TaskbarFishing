"""Convert the supplied artwork to a multi-resolution Windows icon, without changing its design.

Run manually after replacing sprite/app-icon.png; requires Pillow.
The checked-in ICO is used by CMake, so normal builds do not require Python.
"""
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
with Image.open(root / "sprite/app-icon.png") as source:
    source.convert("RGBA").save(
        root / "sprite/app-icon.ico",
        format="ICO",
        sizes=[(size, size) for size in (16, 24, 32, 48, 64, 128, 256)],
    )
