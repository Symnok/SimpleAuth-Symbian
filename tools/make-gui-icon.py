#!/usr/bin/env python3
# Draws tools/otp-qr-gui.ico (the app icon's shield and clock) for the test bench .exe.
# Pillow cannot render icon.svg, so the same shapes are drawn here.
import os
from PIL import Image, ImageDraw

S = 256
img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
k = S / 88.0
d.rounded_rectangle([4 * k, 4 * k, 84 * k, 84 * k], radius=16 * k, fill="#2f6fd0")
shield = [(44, 14), (68, 23), (68, 42), (64, 56), (56, 67), (44, 74), (32, 67), (24, 56), (20, 42), (20, 23)]
d.polygon([(x * k, y * k) for x, y in shield], fill="white")
d.ellipse([30 * k, 30 * k, 58 * k, 58 * k], fill="#2f6fd0")
d.ellipse([33 * k, 33 * k, 55 * k, 55 * k], fill="white")
w = int(3 * k)
d.line([(44 * k, 36 * k), (44 * k, 44 * k), (50 * k, 48 * k)], fill="#2f6fd0", width=w, joint="curve")
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "otp-qr-gui.ico")
img.save(out, sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
print("wrote", out)
