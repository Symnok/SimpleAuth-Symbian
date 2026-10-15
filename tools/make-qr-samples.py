#!/usr/bin/env python3
# Makes tools/qr-test/samples: QR codes of otpauth links spoilt the way a phone camera
# spoils them (small, rotated, skewed, blurred, noisy, dim, off-centre on a busy background),
# plus expected.txt (file -> link) for tools/qr-test. Needs qrcode and Pillow.
import os, random
import qrcode
from PIL import Image, ImageDraw, ImageFilter, ImageEnhance

random.seed(7)
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "qr-test", "samples")
os.makedirs(out, exist_ok=True)

LINKS = [
    "otpauth://totp/SimpleAuth%20Test:tester?secret=APE4PNPMLTII3WCGZTWGBF55VA3WVRPB&issuer=SimpleAuth%20Test",
    "otpauth://totp/Google:alice%40gmail.com?secret=JBSWY3DPEHPK3PXPJBSWY3DPEHPK3PXP&issuer=Google",
    "otpauth://totp/SHA512:bob?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&issuer=SHA512&algorithm=SHA512&digits=8&period=60",
    "otpauth://hotp/HOTP%20Test:tester?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&issuer=HOTP%20Test&counter=0",
]


def code(link, box=8):
    return qrcode.make(link, box_size=box, border=4).get_image().convert("RGB")


def on_background(img, size=(640, 480), scale=0.55, offset=(0, 0)):
    """The code as a camera frame shows it: part of a larger, uneven scene."""
    bg = Image.new("RGB", size, (90, 80, 70))
    d = ImageDraw.Draw(bg)
    for _ in range(40):
        x, y = random.randrange(size[0]), random.randrange(size[1])
        c = tuple(random.randrange(40, 200) for _ in range(3))
        d.rectangle([x, y, x + random.randrange(20, 120), y + random.randrange(20, 120)], fill=c)
    side = int(min(size) * scale)
    q = img.resize((side, side), Image.BILINEAR)
    bg.paste(q, ((size[0] - side) // 2 + offset[0], (size[1] - side) // 2 + offset[1]))
    return bg


def noisy(img, amount):
    px = img.load()
    for y in range(img.height):
        for x in range(img.width):
            n = random.randint(-amount, amount)
            r, g, b = px[x, y]
            px[x, y] = (max(0, min(255, r + n)), max(0, min(255, g + n)), max(0, min(255, b + n)))
    return img


def skew(img, k):
    w, h = img.size
    return img.transform((w, h), Image.PERSPECTIVE, (1, k, -k * h / 2, 0.02, 1, -10, 0.0002, 0.0001),
                         Image.BILINEAR, fillcolor=(90, 80, 70))


samples = []
for i, link in enumerate(LINKS):
    c = code(link)
    samples += [
        ("%d-clean.png" % i, c, link),
        ("%d-frame.png" % i, on_background(c), link),
        ("%d-small.png" % i, on_background(c, (320, 240), 0.8), link),
        ("%d-rotated.png" % i, on_background(c).rotate(17, Image.BILINEAR, fillcolor=(90, 80, 70)), link),
        ("%d-upside-down.png" % i, on_background(c).rotate(180), link),
        ("%d-skewed.png" % i, skew(on_background(c), 0.12), link),
        ("%d-blurred.png" % i, on_background(c).filter(ImageFilter.GaussianBlur(1.2)), link),
        ("%d-noisy.png" % i, noisy(on_background(c), 40), link),
        ("%d-dim.png" % i, ImageEnhance.Contrast(ImageEnhance.Brightness(on_background(c)).enhance(0.45)).enhance(0.5), link),
        ("%d-offcentre.png" % i, on_background(c, scale=0.45, offset=(150, -90)), link),
    ]
# Nothing to find: the decoder must say so rather than invent a result.
samples.append(("none-scene.png", on_background(Image.new("RGB", (10, 10), (120, 120, 120))), ""))

with open(os.path.join(out, "expected.txt"), "w", newline="\n") as f:
    for name, img, link in samples:
        img.save(os.path.join(out, name))
        f.write("%s %s\n" % (name, link))
print("wrote %d samples to %s" % (len(samples), out))
