#!/usr/bin/env python3
"""HOME Menu icon (assets/icon.jpg, 256x256): the bust of a Tweebbo in front of the classic room,
drawn from the game's own avatars. Needs the 3DS brand export (see tools/brandkit.py)."""
import sys
sys.path.insert(0, "tools")
from brandkit import bust, sky, title

im = sky((256, 256))
b = bust(4)
im.alpha_composite(b, ((256 - b.width) // 2, 10))
title(im, "Tweebbo", 46, (128, 248), anchor="ms", outline=4)
im.convert("RGB").save("assets/icon.jpg", quality=95, subsampling=0)
print("assets/icon.jpg: 256x256")
