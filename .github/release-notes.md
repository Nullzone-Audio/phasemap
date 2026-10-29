## Download

| System | File | Installs |
|---|---|---|
| **macOS** 10.13+ (Apple Silicon and Intel) | `…-macOS.pkg` | AU, VST3, CLAP to `/Library/Audio/Plug-Ins`, app to `/Applications` |
| **Windows** 10/11, 64-bit | `…-Windows-Setup.exe` | VST3, CLAP to `C:\Program Files\Common Files`, app to `C:\Program Files\Nullzone Audio` |
| **Linux** x86-64 | `…-Linux-x86_64.tar.gz` | unpack, then `./install.sh` (VST3, CLAP, LV2 to `~/.vst3`, `~/.clap`, `~/.lv2`) |

Then rescan plugins in your DAW.

**The installers are not code-signed yet**, so your system asks once whether you trust them:

- **macOS:** double-click the .pkg. If macOS says it cannot be opened, go to *System Settings → Privacy & Security*, scroll down and click **Open Anyway**.
- **Windows:** if SmartScreen says "Windows protected your PC", click **More info → Run anyway**.

Something wrong? Please [open an issue](https://github.com/Nullzone-Audio/phasemap/issues), German is fine too.

## What's in this version
