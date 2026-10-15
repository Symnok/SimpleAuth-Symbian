#!/usr/bin/env python3
# Prints the current code for an otpauth:// link, independently of the app (Python standard
# library only), to compare with what the phone shows:
#
#   python tools/otp-now.py "otpauth://totp/Test:alice?secret=...&issuer=Test"
#
# TOTP links refresh every second until Ctrl+C; HOTP links print the code for their counter.
import base64, hashlib, hmac, struct, sys, time
from urllib.parse import urlparse, parse_qs, unquote

def code(secret, counter, digits, algorithm):
    mac = hmac.new(secret, struct.pack(">Q", counter), getattr(hashlib, algorithm.lower())).digest()
    offset = mac[-1] & 0x0F
    value = struct.unpack(">I", mac[offset:offset + 4])[0] & 0x7FFFFFFF
    return str(value % 10 ** digits).zfill(digits)

u = urlparse(sys.argv[1])
q = {k: v[0] for k, v in parse_qs(u.query).items()}
raw = q["secret"].upper().replace(" ", "").rstrip("=")
secret = base64.b32decode(raw + "=" * (-len(raw) % 8))
digits, algorithm = int(q.get("digits", 6)), q.get("algorithm", "SHA1")
label = unquote(u.path.lstrip("/"))

if u.netloc == "hotp":
    print(label, code(secret, int(q.get("counter", 0)), digits, algorithm))
    sys.exit(0)
period = int(q.get("period", 30))
try:
    while True:
        now = int(time.time())
        print("\r%s  %s  %2ds left  (UTC %s)" % (label, code(secret, now // period, digits, algorithm),
              period - now % period, time.strftime("%H:%M:%S", time.gmtime(now))), end="", flush=True)
        time.sleep(1)
except KeyboardInterrupt:
    print()
