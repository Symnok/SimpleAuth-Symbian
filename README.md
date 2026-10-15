# SimpleAuth for Symbian

A two-factor authenticator for Nokia phones running **Symbian Anna and Belle**, compatible
with Google Authenticator. It makes the 6-8 digit sign-in codes that sites ask for after the
password, from keys added by scanning a QR code or by typing the setup key.

- Time-based (TOTP, RFC 6238) and counter-based (HOTP, RFC 4226) codes; SHA1, SHA256 and
  SHA512; 6, 7 or 8 digits; any period
- Adding accounts by **camera QR scan**, by setup key, or by pasting an `otpauth://` link
- **Show QR code** for any account, to move it to another authenticator
- Keys **encrypted** (AES-256) under a PIN or passphrase that is required to open the app,
  with auto-lock
- Encrypted **backup and restore** (merging into the accounts already on the phone)
- A clock correction for phones without network time
- Works offline: the app never uses the network

## Requirements

- Symbian Belle (any feature pack) or Symbian Anna, with Qt 4.7.4 or later and Qt Quick
  Components for Symbian (built into Belle)
- Qt Mobility 1.2 for the camera (built into Belle; see [Installing](#installing) for Anna)
- A camera with autofocus is best for scanning; for others see [Camera tips](#camera-tips)

Tested on Nokia phones with Belle FP2, Belle Refresh and Anna, and on the Nokia 808 PureView.

## Installing

Install `SimpleAuth_<version>.sis`. It is self-signed and asks only for camera access
(the `UserEnvironment` capability), which every phone allows for self-signed apps.

The same package works on Belle and on Anna phones that already have Qt Quick Components and
Qt Mobility 1.2. For a phone that lacks them, `build-symbian.cmd installer` also makes
`SimpleAuth_installer_<version>.sis`, which fetches them through Nokia's Smart Installer.

## Using it

### First start

Choose a **PIN** (at least 4 digits) or a **passphrase** (at least 8 characters) and enter it
twice. It is asked for every time the app opens. **There is no reset**: if it is forgotten,
the accounts are lost. Keep each site's recovery codes somewhere safe, and make backups.

Updating from a version before 0.1.8 (which kept the keys unencrypted): the existing
accounts are encrypted at this step and the unencrypted copy is removed.

### Adding an account

Turn on two-factor sign-in at the site (it may be called 2-step verification,
authenticator app or security key). The site shows a QR code. In SimpleAuth tap **+**, then:

- **Scan QR code** and point the camera at the code; the account is added by itself, or
- type the **setup key** the site shows under "can't scan the code?" (letters A-Z and
  digits 2-7; spaces and lower case are fine), with the service and account names; sites
  that use other than SHA1 / 6 digits / 30 seconds say so, and those go under
  **Advanced options**, or
- paste the whole `otpauth://` link.

Then enter the code SimpleAuth shows at the site to finish.

### The account list

- Each account shows its current code and a countdown; the code turns red in its last
  5 seconds.
- **Tap** an account to copy its code.
- **Press and hold** for: Copy code, Next code (counter-based accounts), **Edit**,
  **Show QR code**, Delete.
- **Edit** renames the account (service and account name) and shows its **secret key**
  (hidden until **Show** is tapped) with a **Copy** button, to set the account up in another
  authenticator by hand. The key and the code settings come from the site and cannot be
  changed.
- **Menu**: Settings, About, **Lock now**, **Exit** (the same as Back on this page).

### Settings

- **Clock (UTC) used for codes**, with a **correction** (-10 / -1 / +1 / +10 s). Time-based
  codes are only right when the phone's clock is: compare this clock with an accurate UTC
  clock (e.g. time.is/UTC) and correct it here if it is off. Phones without a SIM card get no
  network time and drift over weeks, so check it now and then.
- **Auto-lock**: at once, after 1 minute (default), 5 or 15 minutes in the background.
- **Change PIN / passphrase** (also switches between the two).
- **Backup**: the folder, **Back up now** and **Restore**, see below.

### Backup and restore

- **Folder**: `SimpleAuth` on a drive of your choice (e.g. `C:\SimpleAuth`,
  `E:\SimpleAuth` on the memory card); it is created on the first backup.
- **Back up now** writes `simpleauth-yyyy-mm-dd-hh-mm.sauth`, encrypted with the PIN or
  passphrase in use at that moment. A second backup within the same minute replaces the
  first.
- **Restore** lists the backups in the folder, newest first. Pick one and enter the PIN or
  passphrase **it was made with** (it may differ from the current one, or come from another
  phone). Accounts missing on this phone are added, and everything is encrypted under this
  phone's PIN. Accounts already here are left as they are (the same key is the same
  account, whatever it is called); for counter-based ones, the higher counter is kept.

Keep copies of the backups off the phone: on a PC, or on another card.

### Camera tips

The scan page has three settings, each remembered on the phone:

- **Rotate**: turns the picture a quarter turn. Firmwares hand the camera picture over
  differently; the app guesses from the Qt version (upright on Qt 4.8 / Belle FP1-FP2,
  turned on Qt 4.7 / Anna, Belle Refresh). The code is read whichever way it shows.
- **Focus**: Automatic, or one of the modes the camera offers (Autofocus, Continuous,
  Macro...). If close-up codes stay blurred, try **Macro**.
- **Zoom** (slider on the right, where the camera offers it): stay back where the camera
  focuses and zoom in instead. This works well on the Nokia 808 PureView.

Tap the picture to focus again. The small line at the top shows what the camera reports.

## Security

- **Keys from the PIN**: PBKDF2-HMAC-SHA256 with a random 16-byte salt. The iteration count
  is measured on the phone when the PIN is set, so that deriving takes about half a second,
  which slows guessing. The result is an AES-256 key and an HMAC-SHA256 key.
- **Storage**: all accounts (as `otpauth://` links) in one blob in the app's private
  settings: AES-256-CTR with a fresh random IV, then HMAC-SHA256 over IV and ciphertext.
  A wrong PIN or a changed file is detected before anything is decrypted.
- **Locking** empties the decrypted list from memory. It happens at start, from the menu,
  and after the auto-lock time in the background.
- **Wrong attempts**: 4 are free, then waits of 30 s, doubling up to an hour, also across
  restarts.
- **Backup files** (`.sauth`): `SAUTHBK1`, the kind of secret, the iteration count and the
  salt, then the accounts sealed as above. They open only with the PIN or passphrase they
  were made with.
- **Limits**: a short PIN stops someone who picks up the phone, but not someone who copies
  the settings or a backup to a PC, where a 4-6 digit PIN can be guessed in minutes. Use a
  passphrase for that. Random numbers come from the phone's kernel random pool.

## Building

Needs the Nokia Qt SDK with the **Qt 4.7.4 for Symbian Anna/Belle** kit
(`D:\QtSDK\Symbian\SDKs\SymbianSR1Qt474`) and Python 3 on the PATH. Qt for Symbian builds
in-source, so the project must be on the same drive as the SDK (D:).

| What | Command |
|---|---|
| Phone package | `build-symbian.cmd` - raises the patch version in `SimpleAuth.pro` first (Symbian will not replace an app with an equal version), then builds `SimpleAuth_<version>.sis`. `build-symbian.cmd installer` also makes the Smart Installer package; `build-symbian.cmd nobump` builds the version in `SimpleAuth.pro` as it is (for a release number set by hand); `build-symbian.cmd clean` removes the build output |
| Core self test | `tools\run-otp-test.cmd` - RFC 4226 / 6238 / 4231 / 4648 / 7914, FIPS-197 and SP 800-38A test vectors, `otpauth://` links, the encrypted store and backup files. `tools\otp-test\release\otp-test.exe --open-backup FILE SECRET` opens any `.sauth` file and prints its accounts (used to check backups made by the Windows Phone version, `SimpleAuth-WP`) |
| QR self test | `tools\run-qr-test.cmd` - makes camera-like QR samples (`tools\make-qr-samples.py`: small, turned, skewed, blurred, noisy, dim, off-centre) and reads them with the app's reader; round-trips the QR encoder through the reader |
| Desktop app | `tools\build-desktop.cmd` builds the app for the PC (desktop Qt 4.7.4 MinGW, which ships the Symbian components). `tools\build-desktop.cmd test` walks every page with `tools\autotest.qml` against a throwaway settings file: setup and migration, adding, the fake camera scan (`SA_SCAN_IMAGES`), Show QR code, lock / unlock / change PIN, backup and restore, wrong-attempt waits; screenshots go to `build-desktop\shots`, and it checks that no key is readable in the settings or the backup |

The self tests and the desktop app use the desktop Qt 4.7.4 MinGW kit
(`D:\QtSDK\Desktop\Qt\4.7.4\mingw`). The QR samples need the Python packages `qrcode` and
`pillow`; the desktop autotest also uses `opencv-python` to read back the QR code the app
draws.

### Test tools for the PC

- `tools\otp-qr-gui.py` - a window that builds test `otpauth://` links from chosen settings
  (random or typed key, TOTP/HOTP, algorithm, digits, period, counter), shows them as a QR
  code to scan and shows the code they make right now, to compare with the phone.
  `tools\build-qr-gui-exe.cmd` packs it into a standalone `tools\otp-qr-gui.exe` that
  runs without Python (PyInstaller in a private virtual environment, `tools\.venv-exe`).
- `tools\otp-now.py "otpauth://..."` - the same live code on the command line.

## Layout

| Path | Contents |
|---|---|
| `src/core` | Plain C++ / QtCore, shared with the self tests: `hash` (SHA-1 via Qt, SHA-256/512, HMAC, PBKDF2), `otp` (Base32, HOTP/TOTP, `otpauth://` links), `aes` (AES-256, CTR), `securerandom`, `vault` (the encrypted store and backup files), `qrcode` (QR encoder) |
| `src/app` | `AccountsModel` (the list, codes, countdown), `AppController` (what QML calls `app`: lock, storage, backup, settings), `QrCodeItem` (draws a QR code) |
| `src/scan` | `QrReader` (ZXing wrapper), `QrScanner` (camera viewfinder, focus, zoom and a decoding thread; on the PC it plays `SA_SCAN_IMAGES` as frames), `FrameSurface` (the camera's video surface) |
| `qml` | The pages |
| `3rdparty/zxing` | The ZXing C++ QR reader, from QZXing 2.3 (the QR part only) |
| `tools` | Self tests, the desktop autotest, the PC test tools |

## License

GPL-3.0-or-later, see `LICENSE`. The SHA-2, AES and QR-encoder code comes from Symbigram
(same license). The QR reader is ZXing, Apache License 2.0, see
`3rdparty/zxing/LICENSE-ZXing.txt`.
