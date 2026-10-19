[README.md](https://github.com/user-attachments/files/33691065/README.md)
<div align="center">

<img src="docs/icon.png" width="128" alt="Pavilion Master icon" />

# Pavilion Master

**A sleek control center for the HP Pavilion 15 Gaming — live temperatures, power draw, CPU power profiles and fan control in one window.**

![Platform](https://img.shields.io/badge/platform-Windows%2010%20%2F%2011%20(x64)-2b2b2b?style=for-the-badge&logo=windows&logoColor=white)
![Built with](https://img.shields.io/badge/built%20with-Qt%206%20%2F%20QML-41cd52?style=for-the-badge&logo=qt&logoColor=white)
![Version](https://img.shields.io/badge/version-0.1-8a5cf6?style=for-the-badge)
![License](https://img.shields.io/badge/license-MIT-b6f23c?style=for-the-badge)

[**⬇ Download**](../../releases/latest) · [Features](#-features) · [Install](#-installation) · [⚠ Warnings](#-important-warnings) · [Credits](#-credits--licenses)

</div>

---

## 📸 Screenshots

<div align="center">

<img src="docs/screenshots/dashboard-acid-green.png" width="780" alt="Pavilion Master dashboard" />

<br/><br/>

| Ghost White | Ultra Violet |
|:---:|:---:|
| <img src="docs/screenshots/dashboard-ghost-white.png" width="380" alt="Ghost White theme" /> | <img src="docs/screenshots/dashboard-ultra-violet.png" width="380" alt="Ultra Violet theme" /> |

| Pick your laptop colour (first launch) | Turbo fan on | Unsupported PC (safe mode) |
|:---:|:---:|:---:|
| <img src="docs/screenshots/accent-picker.png" width="250" alt="Colour variant picker" /> | <img src="docs/screenshots/turbo-on.png" width="250" alt="Turbo fan mode" /> | <img src="docs/screenshots/unsupported.png" width="250" alt="Unsupported processor screen" /> |

</div>

---

## ✨ Features

| | |
|---|---|
| 🌡️ **Live CPU monitoring** | Die temperature and package power, refreshed every second. |
| 🎮 **NVIDIA GPU monitoring** | Temperature and power draw through NVML. Shows `N/A` when there is no NVIDIA GPU. |
| ⚡ **Total power** | CPU + GPU draw combined in one big number. |
| 🚀 **Three power profiles** | **Performance**, **Balanced** and **Quiet**, applied through RyzenADJ with limits tuned per CPU. |
| 🌀 **Turbo fan** | One click to push the fan to 100 %, one click back to automatic. |
| 🔋 **Battery stats** | Charge level, charging state and an *estimated* battery health. |
| 🖥️ **System information** | CPU, GPU, RAM and motherboard at a glance. |
| 🎨 **Colour variants** | Match the app to your laptop: Acid Green, Ghost White or Ultra Violet. |
| 🛡️ **Safe on any PC** | On unsupported hardware the app opens in a monitoring-only mode and **never touches low-level hardware**. |

---

## 💻 Supported hardware

Pavilion Master is built for the **HP Pavilion 15 Gaming** with one of these AMD Ryzen H-series processors:

| CPU | Codename | Performance | Balanced | Quiet |
|---|---|:---:|:---:|:---:|
| Ryzen 7 **3750H** | Picasso | 38 / 45 / 40 W · 92 °C | 25 / 35 / 28 W · 85 °C | 15 / 20 / 17 W · 75 °C |
| Ryzen 5 **4600H** | Renoir | 50 / 60 / 54 W · 92 °C | 35 / 45 / 38 W · 85 °C | 25 / 30 / 27 W · 75 °C |
| Ryzen 7 **4800H** | Renoir | 54 / 65 / 58 W · 92 °C | 35 / 45 / 38 W · 85 °C | 28 / 35 / 30 W · 75 °C |
| Ryzen 5 **5600H** | Cezanne | 50 / 60 / 54 W · 92 °C | 35 / 45 / 38 W · 85 °C | 22 / 28 / 24 W · 75 °C |
| Ryzen 7 **5800H** | Cezanne | 54 / 65 / 58 W · 92 °C | 35 / 45 / 38 W · 85 °C | 25 / 32 / 28 W · 75 °C |

<sub>Values are sustained (STAPM) / fast / slow power limits and the Tctl temperature limit. They are sensible starting points based on AMD's published ranges, **not** values measured on every laptop.</sub>

> [!NOTE]
> **Any other PC** — different CPU, different laptop, Intel, desktop — is treated as *unsupported*. The app still installs and opens, shows an "Unsupported Processor" screen, and the profile and fan controls are disabled. No WinRing0 driver is loaded and no hardware is written to.

**Requirements:** Windows 10 (1809) or Windows 11, 64-bit. Nothing else needs to be installed — the Qt runtime is bundled.

---

## 📦 Installation

1. Go to the [**Releases**](../../releases/latest) page and download **`PavilionMaster-Bundle.zip`**.
2. **Extract the whole ZIP** (don't run the installer from inside the archive).
3. Double-click **`Install.bat`** and accept the administrator prompt.

The installer copies Pavilion Master to `C:\Program Files\Pavilion Master`, adds Start menu and desktop shortcuts, sets the app to always run as administrator, and launches it.

<details>
<summary><b>Windows says "Windows protected your PC" (SmartScreen)</b></summary>

The files are not code-signed, so Windows shows an "Unknown publisher" warning. Click **More info → Run anyway** if you trust the download. Only download Pavilion Master from this repository's Releases page.
</details>

### Uninstall

Use **Settings → Apps → Pavilion Master → Uninstall**, or run `Uninstall.bat` from the install folder. This removes the app, shortcuts, saved settings and the WinRing0 driver service.

---

## ⚠️ Important warnings

> [!CAUTION]
> **Use at your own risk.** Pavilion Master changes CPU power limits, temperature limits and fan behaviour. Aggressive settings can raise temperatures, cause instability or crashes, shorten component life, and may affect your warranty. The software is provided *as is*, without any warranty (see [LICENSE](LICENSE)). The author is not responsible for any damage or data loss.

> [!WARNING]
> **It installs a kernel driver with a known vulnerability.**
> On supported laptops the app loads **WinRing0** (`WinRing0x64.sys`), a third-party driver that gives programs raw access to CPU registers, I/O ports and the fan controller. Microsoft and antivirus vendors flag it because older versions of this driver family have a publicly known privilege-escalation vulnerability ([CVE-2020-14979](https://nvd.nist.gov/vuln/detail/CVE-2020-14979)). While the driver is installed, malware that is already running on your PC could try to abuse it.
> - Only use Pavilion Master on a PC you control and keep protected.
> - If you stop using the app, **uninstall it** — that removes the driver service.
> - Windows Security, Memory Integrity (HVCI) or the Microsoft vulnerable-driver blocklist may **quarantine or refuse to load** the driver. The app then opens in limited mode with the controls disabled. Disabling those protections is **your decision and your risk** — they exist for good reason.

> [!IMPORTANT]
> **Administrator rights are required.** Loading the driver and changing power limits needs an elevated process, so the app is set to always run as administrator and Windows will ask for confirmation each time.

> [!NOTE]
> **Unofficial project.** Pavilion Master is an independent fan-made tool. It is **not affiliated with, endorsed by or supported by HP, AMD, NVIDIA or Apple**. Support requests should not be sent to them.

**Also good to know**

- Fan control writes to the laptop's embedded controller and is designed for the HP Pavilion 15 Gaming only. The fan never goes below 38 %, and **Turbo** sets it to 100 %.
- Battery health is an **estimate**, calculated against a fixed design capacity.
- Power profiles reset when the PC restarts or sleeps; re-apply them after boot.
- Closing the app does not revert the profile that is currently applied. Pick **Balanced** before you quit if unsure.

---

## 🛠️ Troubleshooting

<details>
<summary><b>The controls are greyed out / "Unsupported Processor" appears</b></summary>

Your CPU isn't one of the five supported models, or the driver couldn't load. Check `control_center.log` in the install folder — it says either `Unsupported CPU` or `FanControl initialization failed`.
</details>

<details>
<summary><b>The CPU shows "--" and the fan button does nothing on a supported laptop</b></summary>

The WinRing0 driver was probably blocked or deleted by Windows Security. Open **Windows Security → Virus & threat protection → Protection history**, review the entry for `WinRing0x64.sys`, and restore it only if you accept the risks described above. Then restart the app.

To check the driver service in a terminal: `sc query WinRing0_1_2_0`
</details>

<details>
<summary><b>D-GPU shows N/A</b></summary>

No NVIDIA GPU was found or the NVIDIA driver isn't installed. Update or reinstall your NVIDIA driver. Pavilion Master uses the driver's own `nvml.dll` when it exists.
</details>

<details>
<summary><b>Where are the logs?</b></summary>

`C:\Program Files\Pavilion Master\control_center.log`. Please attach it when you report a bug.
</details>

---

## 🧰 Building from source

The source code will be published here after launch. It is a Qt 6 (Qt Quick / QML) + C++17 project built with CMake.

---

## 🙏 Credits & licenses

Pavilion Master stands on the work of others. Huge thanks to everyone below.

### Created by
**@MIDO5659** — design and development.

### Third-party components

| Component | Credit | License | Used for |
|---|---|---|---|
| **[RyzenAdj](https://github.com/FlyGoat/RyzenAdj)** | **FlyGoat** and the RyzenAdj contributors | LGPL-3.0 | Applying the Ryzen power and temperature limits (`ryzenadj.exe`, `libryzenadj.dll`). Run as a separate program. |
| **NVML** (NVIDIA Management Library) | © **NVIDIA Corporation** | NVIDIA's own license (ships with the NVIDIA driver / CUDA Toolkit) | Reading NVIDIA GPU temperature and power (`nvml.dll`, `nvml.h`). |
| **[WinRing0](https://openlibsys.org/)** | **OpenLibSys.org** | BSD-style | Low-level access to CPU registers and the fan controller. |
| **[Qt 6](https://www.qt.io/)** | © The Qt Company Ltd. and contributors | LGPL-3.0 (dynamically linked) | The UI framework. Qt's source code is available from [qt.io](https://www.qt.io/download-open-source). |
| **MinGW-w64 runtime** (`libgcc`, `libstdc++`, `winpthreads`) | GNU Project and MinGW-w64 contributors | GPL-3.0 with the GCC Runtime Library Exception | C++ runtime for the compiled app. |

### Fonts

| Font | Credit | Used for |
|---|---|---|
| **SF Pro Display** | © 2015-2020 **Apple Inc.** | General interface text. |
| **AKONY** | © 2023 **grinfleim**, designed by **Vladimir Klenov** | "TURBO ON" / "AUTO" and the large power numbers. |
| **Angled** | **Artyway** — designed by **Yehor Lisnyi** ([Creative Market](https://creativemarket.com/artyway)) | CPU / GPU temperature and "TOTAL" labels. |

Fonts remain the property of their respective owners and are used under their own license terms; they are **not** covered by this project's MIT license and may not be extracted or reused from this application without the owners' permission.

### Trademarks
HP and Pavilion are trademarks of HP Inc. AMD and Ryzen are trademarks of Advanced Micro Devices, Inc. NVIDIA, GeForce and NVML are trademarks and/or registered trademarks of NVIDIA Corporation. Apple and SF Pro are trademarks of Apple Inc. Windows is a trademark of Microsoft Corporation. All other trademarks belong to their respective owners. They are used here only to describe compatibility.

### Pavilion Master license
The original source code of Pavilion Master is released under the **[MIT License](LICENSE)** © 2026 MIDO5659. Third-party components keep their own licenses as listed above.

---

<div align="center">

Made with 💚💜 for the HP Pavilion 15 Gaming.

</div>
