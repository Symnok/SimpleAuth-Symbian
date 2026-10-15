#!/usr/bin/env python3
# A test bench for SimpleAuth on the PC: builds an otpauth:// link from the chosen settings,
# shows it as a QR code (to scan with Google Authenticator or, later, SimpleAuth) and shows
# the code it produces right now, to compare with the phone.
#
#   python tools/otp-qr-gui.py          (needs: pip install qrcode pillow)
#
# For test keys only: the key is shown on screen and in the link.
import base64, hashlib, hmac, os, re, struct, sys, time
import tkinter as tk
from tkinter import filedialog, ttk
from urllib.parse import quote

import qrcode
from PIL import ImageTk

B32 = re.compile(r"^[A-Z2-7]+=*$")


def normalize_secret(text):
    """The key as sites print it -> plain upper-case Base32, or None if not Base32."""
    s = re.sub(r"[\s-]", "", text).upper()
    return s.rstrip("=") if s and B32.match(s) else None


def otp(secret_b32, counter, digits, algorithm):
    key = base64.b32decode(secret_b32 + "=" * (-len(secret_b32) % 8))
    mac = hmac.new(key, struct.pack(">Q", counter), getattr(hashlib, algorithm.lower())).digest()
    offset = mac[-1] & 0x0F
    value = struct.unpack(">I", mac[offset:offset + 4])[0] & 0x7FFFFFFF
    return str(value % 10 ** digits).zfill(digits)


def grouped(code):
    half = len(code) // 2
    return code[:half] + " " + code[half:]


class App:
    def __init__(self, root):
        self.root = root
        root.title("SimpleAuth test QR generator")
        # The .exe unpacks its bundled files to sys._MEIPASS; the script finds the icon beside it.
        icon = os.path.join(getattr(sys, "_MEIPASS", os.path.dirname(os.path.abspath(__file__))), "otp-qr-gui.ico")
        if os.path.exists(icon):
            root.iconbitmap(default=icon)
        root.resizable(False, False)
        self.qr_image = None
        self.link = ""

        form = ttk.Frame(root, padding=10)
        form.grid(row=0, column=0, sticky="n")
        right = ttk.Frame(root, padding=10)
        right.grid(row=0, column=1, sticky="n")

        self.issuer = tk.StringVar(value="SimpleAuth Test")
        self.account = tk.StringVar(value="tester")
        self.secret = tk.StringVar()
        self.key_bytes = tk.StringVar(value="20")
        self.kind = tk.StringVar(value="totp")
        self.algorithm = tk.StringVar(value="SHA1")
        self.digits = tk.StringVar(value="6")
        self.period = tk.StringVar(value="30")
        self.counter = tk.StringVar(value="0")

        r = 0
        ttk.Label(form, text="Service (issuer)").grid(row=r, column=0, sticky="w", pady=2)
        ttk.Entry(form, textvariable=self.issuer, width=34).grid(row=r, column=1, columnspan=2, sticky="we")
        r += 1
        ttk.Label(form, text="Account").grid(row=r, column=0, sticky="w", pady=2)
        ttk.Entry(form, textvariable=self.account, width=34).grid(row=r, column=1, columnspan=2, sticky="we")

        r += 1
        ttk.Label(form, text="Secret key (Base32)").grid(row=r, column=0, sticky="w", pady=(10, 2))
        ttk.Entry(form, textvariable=self.secret, width=34, font=("Consolas", 10)).grid(
            row=r, column=1, columnspan=2, sticky="we", pady=(10, 2))
        r += 1
        ttk.Label(form, text="Random key length").grid(row=r, column=0, sticky="w", pady=2)
        ttk.Combobox(form, textvariable=self.key_bytes, state="readonly", width=24,
                     values=["10 bytes (16 chars)", "20 bytes (32 chars)", "32 bytes (52 chars)", "64 bytes (103 chars)"]
                     ).grid(row=r, column=1, sticky="w")
        self.key_bytes.set("20 bytes (32 chars)")
        ttk.Button(form, text="New random key", command=self.random_key).grid(row=r, column=2, sticky="e", padx=(6, 0))
        r += 1
        self.secret_note = ttk.Label(form, text="", foreground="#b00020")
        self.secret_note.grid(row=r, column=1, columnspan=2, sticky="w")

        r += 1
        ttk.Separator(form).grid(row=r, column=0, columnspan=3, sticky="we", pady=8)
        r += 1
        ttk.Label(form, text="Type").grid(row=r, column=0, sticky="w", pady=2)
        kinds = ttk.Frame(form)
        kinds.grid(row=r, column=1, columnspan=2, sticky="w")
        ttk.Radiobutton(kinds, text="Time-based (TOTP)", value="totp", variable=self.kind).pack(side="left")
        ttk.Radiobutton(kinds, text="Counter-based (HOTP)", value="hotp", variable=self.kind).pack(side="left", padx=(10, 0))
        r += 1
        ttk.Label(form, text="Algorithm").grid(row=r, column=0, sticky="w", pady=2)
        ttk.Combobox(form, textvariable=self.algorithm, values=["SHA1", "SHA256", "SHA512"], state="readonly",
                     width=10).grid(row=r, column=1, sticky="w")
        r += 1
        ttk.Label(form, text="Digits").grid(row=r, column=0, sticky="w", pady=2)
        ttk.Combobox(form, textvariable=self.digits, values=["6", "7", "8"], state="readonly",
                     width=10).grid(row=r, column=1, sticky="w")
        r += 1
        self.period_label = ttk.Label(form, text="Period, seconds")
        self.period_label.grid(row=r, column=0, sticky="w", pady=2)
        self.period_box = ttk.Combobox(form, textvariable=self.period, values=["15", "30", "60", "90", "120"], width=10)
        self.period_box.grid(row=r, column=1, sticky="w")
        r += 1
        self.counter_label = ttk.Label(form, text="Counter")
        self.counter_label.grid(row=r, column=0, sticky="w", pady=2)
        counter_row = ttk.Frame(form)
        counter_row.grid(row=r, column=1, columnspan=2, sticky="w")
        self.counter_box = ttk.Spinbox(counter_row, textvariable=self.counter, from_=0, to=10 ** 9, width=10)
        self.counter_box.pack(side="left")
        self.counter_next = ttk.Button(counter_row, text="Next code", command=self.next_counter)
        self.counter_next.pack(side="left", padx=(6, 0))

        r += 1
        ttk.Separator(form).grid(row=r, column=0, columnspan=3, sticky="we", pady=8)
        r += 1
        ttk.Label(form, text="Current code").grid(row=r, column=0, sticky="w")
        self.code_label = ttk.Label(form, text="", font=("Segoe UI", 26, "bold"), foreground="#1f5fbf")
        self.code_label.grid(row=r, column=1, columnspan=2, sticky="w")
        r += 1
        self.time_label = ttk.Label(form, text="", foreground="#555555")
        self.time_label.grid(row=r, column=1, columnspan=2, sticky="w")
        r += 1
        self.progress = ttk.Progressbar(form, length=260, maximum=30)
        self.progress.grid(row=r, column=1, columnspan=2, sticky="w", pady=(2, 0))

        r += 1
        ttk.Label(form, text="Link").grid(row=r, column=0, sticky="nw", pady=(10, 0))
        self.link_text = tk.Text(form, width=44, height=4, wrap="char", font=("Consolas", 9))
        self.link_text.grid(row=r, column=1, columnspan=2, sticky="we", pady=(10, 0))
        r += 1
        buttons = ttk.Frame(form)
        buttons.grid(row=r, column=1, columnspan=2, sticky="w", pady=(6, 0))
        ttk.Button(buttons, text="Copy link", command=self.copy_link).pack(side="left")
        ttk.Button(buttons, text="Copy key", command=self.copy_key).pack(side="left", padx=(6, 0))
        ttk.Button(buttons, text="Save QR as PNG...", command=self.save_png).pack(side="left", padx=(6, 0))
        self.status = ttk.Label(form, text="", foreground="#2e7d32")
        self.status.grid(row=r + 1, column=1, columnspan=2, sticky="w")

        self.qr_label = ttk.Label(right)
        self.qr_label.pack()
        ttk.Label(right, text="Scan with Google Authenticator\nor type the key into SimpleAuth",
                  justify="center", foreground="#555555").pack(pady=(6, 0))

        for v in (self.issuer, self.account, self.secret, self.kind, self.algorithm, self.digits,
                  self.period, self.counter):
            v.trace_add("write", lambda *_: self.rebuild())
        self.random_key()
        self.tick()

    # -- actions --

    def random_key(self):
        n = int(self.key_bytes.get().split()[0])
        self.secret.set(base64.b32encode(os.urandom(n)).decode().rstrip("="))

    def next_counter(self):
        try:
            self.counter.set(str(int(self.counter.get()) + 1))
        except ValueError:
            self.counter.set("0")

    def copy_link(self):
        if self.link:
            self.root.clipboard_clear()
            self.root.clipboard_append(self.link)
            self.flash("Link copied")

    def copy_key(self):
        s = normalize_secret(self.secret.get())
        if s:
            self.root.clipboard_clear()
            self.root.clipboard_append(s)
            self.flash("Key copied")

    def save_png(self):
        if not self.link:
            return
        name = re.sub(r"[^\w.-]+", "_", "%s_%s" % (self.issuer.get(), self.account.get())).strip("_") or "otp"
        path = filedialog.asksaveasfilename(defaultextension=".png", initialfile=name + ".png",
                                            filetypes=[("PNG image", "*.png")])
        if path:
            qrcode.make(self.link, box_size=10, border=4).save(path)
            self.flash("Saved " + os.path.basename(path))

    def flash(self, text):
        self.status.config(text=text)
        self.root.after(2500, lambda: self.status.config(text=""))

    # -- state --

    def settings(self):
        """The current settings, or an error string."""
        secret = normalize_secret(self.secret.get())
        if not secret:
            return "Key must be Base32: letters A-Z and digits 2-7 (spaces are ignored)."
        try:
            period = int(self.period.get())
            if not 1 <= period <= 3600:
                raise ValueError
        except ValueError:
            return "Period must be 1-3600 seconds."
        try:
            counter = int(self.counter.get())
            if counter < 0:
                raise ValueError
        except ValueError:
            return "Counter must be a whole number."
        return dict(secret=secret, period=period, counter=counter, digits=int(self.digits.get()),
                    algorithm=self.algorithm.get(), hotp=self.kind.get() == "hotp")

    def rebuild(self):
        hotp = self.kind.get() == "hotp"
        for w in (self.period_label, self.period_box):
            w.grid() if not hotp else w.grid_remove()
        for w in (self.counter_label, self.counter_box.master):
            w.grid() if hotp else w.grid_remove()

        s = self.settings()
        if isinstance(s, str):
            self.secret_note.config(text=s)
            self.link = ""
            self.set_link_text("")
            self.qr_label.config(image="", text="(no valid key)")
            return
        self.secret_note.config(text="")

        issuer, account = self.issuer.get().strip(), self.account.get().strip()
        label = quote(account, safe="@")
        if issuer:
            label = quote(issuer, safe="") + ":" + label
        link = "otpauth://%s/%s?secret=%s" % ("hotp" if s["hotp"] else "totp", label, s["secret"])
        if issuer:
            link += "&issuer=" + quote(issuer, safe="")
        if s["algorithm"] != "SHA1":
            link += "&algorithm=" + s["algorithm"]
        if s["digits"] != 6:
            link += "&digits=%d" % s["digits"]
        if s["hotp"]:
            link += "&counter=%d" % s["counter"]
        elif s["period"] != 30:
            link += "&period=%d" % s["period"]

        if link != self.link:
            self.link = link
            self.set_link_text(link)
            img = qrcode.make(link, box_size=6, border=3).get_image().convert("RGB")
            self.qr_image = ImageTk.PhotoImage(img)
            self.qr_label.config(image=self.qr_image, text="")
        self.update_code()

    def set_link_text(self, text):
        self.link_text.config(state="normal")
        self.link_text.delete("1.0", "end")
        self.link_text.insert("1.0", text)
        self.link_text.config(state="disabled")

    def update_code(self):
        s = self.settings()
        if isinstance(s, str):
            self.code_label.config(text="")
            self.time_label.config(text="")
            self.progress["value"] = 0
            return
        now = int(time.time())
        utc = time.strftime("%H:%M:%S", time.gmtime(now))
        if s["hotp"]:
            code = otp(s["secret"], s["counter"], s["digits"], s["algorithm"])
            self.time_label.config(text="counter %d  |  UTC %s" % (s["counter"], utc))
            self.progress["value"] = 0
            self.code_label.config(text=grouped(code), foreground="#1f5fbf")
        else:
            left = s["period"] - now % s["period"]
            code = otp(s["secret"], now // s["period"], s["digits"], s["algorithm"])
            self.time_label.config(text="%d s left  |  UTC %s" % (left, utc))
            self.progress.config(maximum=s["period"], value=left)
            self.code_label.config(text=grouped(code), foreground="#c62828" if left <= 5 else "#1f5fbf")

    def tick(self):
        self.update_code()
        self.root.after(250, self.tick)


if __name__ == "__main__":
    root = tk.Tk()
    App(root)
    root.mainloop()
