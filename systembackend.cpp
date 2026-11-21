#include "SystemBackend.h"
#include "PowerPlans.h"
#include <windows.h>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QStringList>
#include <QSet>
#include <QRegularExpression>
#include <QObject>
#include <QTimer>
#include <QString>
#include <QIODevice>
#include <QMessageBox>
#include <QGuiApplication>
#include <QSettings>

namespace {
// Must match the `key` values of accentVariants in Frame_1.qml
const QStringList kAccentKeys = { "acid_green", "ghost_white", "ultra_violet" };

// ---------------------------------------------------------------------------
// Supported CPUs and their power profiles  { stapm W, fast W, slow W, tctl C }
//
// Basis (AMD specs, via Notebookcheck): Picasso H is 35 W default / 12-35 W configurable;
// Renoir and Cezanne H are 45 W default / 35-54 W configurable; all have a 105 C max.
// Tctl tiers (92 / 85 / 75 C) stay well under that for every chip.
//
//  Performance : top of AMD's range for the 45 W parts (3750H keeps its original
//                over-spec values), with extra short-term boost headroom.
//  Balanced    : 35 W sustained (AMD's cTDP floor, where efficiency is best) with the
//                chip's default 45 W as the short boost.
//  Quiet       : well below the floor; 6-core parts and the more efficient Zen 3
//                parts can go lower than the 8-core Zen 2 part.
//
// These are starting points chosen from the specs, not values measured on every
// laptop - tune them here if a model runs too hot or too cool.
// ---------------------------------------------------------------------------
const CpuSpec kSupportedCpus[] = {
    //  model    codename    performance        balanced          quiet
    { "3750H", "Picasso", { 38, 45, 40, 92 }, { 25, 35, 28, 85 }, { 15, 20, 17, 75 } },
    { "4600H", "Renoir",  { 50, 60, 54, 92 }, { 35, 45, 38, 85 }, { 25, 30, 27, 75 } },
    { "4800H", "Renoir",  { 54, 65, 58, 92 }, { 35, 45, 38, 85 }, { 28, 35, 30, 75 } },
    { "5600H", "Cezanne", { 50, 60, 54, 92 }, { 35, 45, 38, 85 }, { 22, 28, 24, 75 } },
    { "5800H", "Cezanne", { 54, 65, 58, 92 }, { 35, 45, 38, 85 }, { 25, 32, 28, 75 } },
};
}

SystemBackend::SystemBackend(QObject* parent) : QObject(parent), m_initializationFailed(false), m_fanAvailable(false), m_totalWatts("--W") {
    // Saved laptop color variant (stored under HKCU\Software\MIDO5659\GControlCenter)
    {
        QSettings settings("MIDO5659", "GControlCenter");
        const QString saved = settings.value("appearance/accentVariant").toString();
        if (kAccentKeys.contains(saved))
            m_accentVariant = saved;
    }

    // Nothing below may stop the app from launching: on unsupported or non-NVIDIA hardware
    // the window must still open so the "Unsupported Processor" screen can be shown.

    // NVIDIA GPU readings are optional
    initNvml();

    // Detect the CPU first - this decides whether we may touch the hardware at all
    readStaticSystemInfo();
    readBatteryHealth();

    // WinRing0 (raw MSR / PCI / EC access) is AMD- and HP-specific, so it is only
    // initialized on a supported CPU. On anything else the app stays read-only/greyed out.
    if (!m_cpuSpec) {
        qWarning() << "Unsupported CPU - low-level hardware access (WinRing0) is NOT initialized";
        m_fanAvailable = false;
    } else if (!m_fan.Initialize()) {
        qCritical() << "FanControl initialization failed";
        m_fanAvailable = false;
    } else {
        qInfo() << "FanControl initialized successfully";
        m_fanAvailable = true;
    }

    // 1-second refresh timer for all live data
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SystemBackend::refresh);
    m_timer->start(1000);
}

SystemBackend::~SystemBackend() {}

bool SystemBackend::initNvml() {
    HMODULE lib = LoadLibraryA("nvml.dll");
    if (!lib) {
        // Older drivers keep it here instead of System32
        const QByteArray pf = qgetenv("ProgramFiles");
        if (!pf.isEmpty())
            lib = LoadLibraryA((pf + "\\NVIDIA Corporation\\NVSMI\\nvml.dll").constData());
    }
    if (!lib) {
        qWarning() << "nvml.dll not found (no NVIDIA driver?) - D-GPU readings disabled";
        return false;
    }

    m_nvml.lib            = lib;
    m_nvml.init           = reinterpret_cast<decltype(m_nvml.init)>(GetProcAddress(lib, "nvmlInit_v2"));
    m_nvml.getHandle      = reinterpret_cast<decltype(m_nvml.getHandle)>(GetProcAddress(lib, "nvmlDeviceGetHandleByIndex_v2"));
    m_nvml.getName        = reinterpret_cast<decltype(m_nvml.getName)>(GetProcAddress(lib, "nvmlDeviceGetName"));
    m_nvml.getTemperature = reinterpret_cast<decltype(m_nvml.getTemperature)>(GetProcAddress(lib, "nvmlDeviceGetTemperature"));
    m_nvml.getPowerUsage  = reinterpret_cast<decltype(m_nvml.getPowerUsage)>(GetProcAddress(lib, "nvmlDeviceGetPowerUsage"));

    if (!m_nvml.init || !m_nvml.getHandle || !m_nvml.getName || !m_nvml.getTemperature || !m_nvml.getPowerUsage) {
        qWarning() << "nvml.dll is missing expected functions - D-GPU readings disabled";
        return false;
    }
    if (m_nvml.init() != NVML_SUCCESS) {
        qWarning() << "NVML initialization failed - D-GPU readings disabled";
        return false;
    }
    if (m_nvml.getHandle(0, &m_gpu) != NVML_SUCCESS) {
        qWarning() << "No NVIDIA GPU found via NVML - D-GPU readings disabled";
        return false;
    }

    m_nvmlReady = true;
    qInfo() << "NVML initialized successfully";
    return true;
}

void SystemBackend::saveAccentVariant(const QString& key) {
    if (!kAccentKeys.contains(key) || key == m_accentVariant)
        return;
    m_accentVariant = key;
    QSettings settings("MIDO5659", "GControlCenter");
    settings.setValue("appearance/accentVariant", key);
    qInfo() << "Accent variant saved:" << key;
    emit accentVariantChanged();
}

void SystemBackend::readStaticSystemInfo() {
    wchar_t buffer[255];
    DWORD bufferSize = sizeof(buffer);
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"ProcessorNameString", NULL, NULL,
            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS)
            m_cpuName = QString::fromWCharArray(buffer).trimmed();
        RegCloseKey(hKey);
    }

    wchar_t bufferMOBO[255];
    DWORD bufferSizeMOBO = sizeof(bufferMOBO);
    HKEY Mhkey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\BIOS",
        0, KEY_READ, &Mhkey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(Mhkey, L"BaseBoardProduct", NULL, NULL,
            (LPBYTE)bufferMOBO, &bufferSizeMOBO) == ERROR_SUCCESS)
            m_moboName = QString::fromWCharArray(bufferMOBO).trimmed();
        RegCloseKey(Mhkey);
    }

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    double ramGB = statex.ullTotalPhys / 1024.0 / 1024.0 / 1024.0;
    m_ramSize = QString::number(ramGB, 'f', 2) + " GB";

    // Decide support from the CPU model (this is what unlocks the controls)
    m_cpuSpec = findCpuSpec(m_cpuName);
    if (m_cpuSpec)
        qInfo() << "Supported CPU detected:" << m_cpuSpec->model << m_cpuSpec->codename;
    else
        qWarning() << "Unsupported CPU:" << m_cpuName;

    if (m_nvmlReady) {
        char gpuname[64];
        m_nvml.getName(m_gpu, gpuname, 64);
        m_gpuName = QString::fromUtf8(gpuname);
    } else {
        m_gpuName = "N/A";
    }
}

void SystemBackend::refresh() {
    int cpuPowerWatts = 0;
    bool cpuPowerAvailable = false;
    if (m_fanAvailable) {
        double watts = m_fan.GetPackageWattage();
        m_cpuWatts = QString::number((int)watts) + "W";
        cpuPowerWatts = (int)watts;
        cpuPowerAvailable = true;

        // Read CPU die temperature directly via MSR (WinRing0)
        double cpuTemp = m_fan.GetCpuTemp();
        if (cpuTemp > 0.0) {
            m_cpuTemp  = "CPU: "  + QString::number((int)cpuTemp) + "°C";
            m_igpuTemp = "iGPU: " + QString::number((int)cpuTemp) + "°C"; // Vega is on same die
            m_igpuWatts = m_cpuWatts;
        } else {
            // If cpuTemp is not valid, reset to defaults
            m_cpuWatts = "--W";
            m_cpuTemp = "CPU: --°C";
            m_igpuTemp = "iGPU: --°C";
            m_igpuWatts = "--W";
            cpuPowerAvailable = false; // Treat as unavailable if temp invalid (as per original logic)
            cpuPowerWatts = 0;
        }
    } else {
        // FanControl not available: reset to defaults
        m_cpuWatts = "--W";
        m_cpuTemp = "CPU: --°C";
        m_igpuTemp = "iGPU: --°C";
        m_igpuWatts = "--W";
    }

    int gpuPowerWatts = 0;
    bool gpuPowerAvailable = false;
    // D-GPU via NVML
    if (m_nvmlReady) {
        unsigned int gt = 0;
        m_nvml.getTemperature(m_gpu, NVML_TEMPERATURE_GPU, &gt);
        m_dgpuTemp = "D-GPU: " + QString::number(gt) + "°C";

        unsigned int mw = 0;
        m_nvml.getPowerUsage(m_gpu, &mw);
        m_dgpuWatts = QString::number((int)(mw / 1000)) + "W";
        gpuPowerWatts = (int)(mw / 1000);
        gpuPowerAvailable = true;
    } else {
        m_dgpuTemp  = "D-GPU: N/A";
        m_dgpuWatts = "N/A";
    }

    SYSTEM_POWER_STATUS sps;
    GetSystemPowerStatus(&sps);
    QString state = (sps.ACLineStatus == 1) ? "CHARGING" : "DISCHARGING";
    m_battCharge = QString::number((int)sps.BatteryLifePercent) + "% [" + state + "]";

    // Compute total power
    int totalPowerWatts = 0;
    if (cpuPowerAvailable) {
        totalPowerWatts += cpuPowerWatts;
    }
    if (gpuPowerAvailable) {
        totalPowerWatts += gpuPowerWatts;
    }
    if (cpuPowerAvailable || gpuPowerAvailable) {
        m_totalWatts = QString::number(totalPowerWatts) + "W";
    } else {
        m_totalWatts = "--W";
    }

    emit dataChanged();
}

void SystemBackend::readBatteryHealth() {
    QProcess p;
    p.start("powershell", {"-command",
        "$v=(Get-CimInstance -Namespace root/wmi -ClassName BatteryFullChargedCapacity).FullChargedCapacity; $v | Out-File battery_data.txt -Encoding ASCII -Force"});
    p.waitForFinished(5000);

    QFile f("battery_data.txt");
    if (f.open(QIODevice::ReadOnly)) {
        QString line = QTextStream(&f).readAll().trimmed();
        bool ok;
        long cap = line.toLong(&ok);
        if (ok && cap > 0) {
            double health = (cap / 52553.0) * 100.0;
            if (health > 100) health = 100;
            m_battHealth = QString::number((int)health) + "% [ESTIMATED]";
        } else {
            m_battHealth = "N/A";
        }
    } else {
        m_battHealth = "N/A";
    }
}

const CpuSpec* SystemBackend::findCpuSpec(const QString& fullName) const {
    // e.g. "AMD Ryzen 7 5800H with Radeon Graphics". The word boundaries keep
    // "5800H" from matching "5800HS" / "5800HX", which have different limits.
    if (!fullName.contains(QStringLiteral("Ryzen"), Qt::CaseInsensitive))
        return nullptr;
    for (const CpuSpec& spec : kSupportedCpus) {
        const QRegularExpression re(QStringLiteral("\\b%1\\b").arg(QLatin1String(spec.model)),
                                    QRegularExpression::CaseInsensitiveOption);
        if (fullName.contains(re))
            return &spec;
    }
    return nullptr;
}

void SystemBackend::applyProfile(const PowerProfile& p, const QString& modeName) {
    const QByteArray cmd = QByteArray("ryzenadj.exe")
        + " --stapm-limit=" + QByteArray::number(p.stapmW * 1000)
        + " --fast-limit="  + QByteArray::number(p.fastW  * 1000)
        + " --slow-limit="  + QByteArray::number(p.slowW  * 1000)
        + " --tctl-temp="   + QByteArray::number(p.tctlC);
    qInfo() << "Applying" << modeName << "profile for" << m_cpuSpec->model << ":" << cmd;
    WinExec(cmd.constData(), SW_HIDE);
    m_perfMode = modeName;
    emit dataChanged();
}

void SystemBackend::setPerformanceMode() {
    if (!m_cpuSpec) return; // unsupported CPU: never touch hardware
    applyProfile(m_cpuSpec->performance, QStringLiteral("PERFORMANCE"));
    if (!PowerPlans::applyPerformance())
        qWarning() << "Could not switch the Windows power plan for PERFORMANCE mode";
}

void SystemBackend::setBalancedMode() {
    if (!m_cpuSpec) return; // unsupported CPU: never touch hardware
    applyProfile(m_cpuSpec->balanced, QStringLiteral("BALANCED"));
    if (!PowerPlans::applyBalanced())
        qWarning() << "Could not switch the Windows power plan for BALANCED mode";
}

void SystemBackend::setQuietMode() {
    if (!m_cpuSpec) return; // unsupported CPU: never touch hardware
    applyProfile(m_cpuSpec->quiet, QStringLiteral("QUIET"));
    if (!PowerPlans::applyQuiet())
        qWarning() << "Could not switch the Windows power plan for QUIET mode";
}

void SystemBackend::toggleFan() {
    if (!m_cpuSpec) return; // unsupported CPU: never touch hardware
    if (!m_fanAvailable) {
        // Fan control not available; do nothing
        return;
    }
    if (m_fanMode == "AUTO") {
        m_fan.SetFanSpeed(100);
        m_fanMode = "TURBO";
    } else {
        m_fan.SetAutoMode();
        m_fanMode = "AUTO";
    }
    emit dataChanged();
}

// Utility function to parse CPU name and extract just the processor model
QString SystemBackend::parseCpuName(const QString& fullName) const {
    if (fullName == "Unknown") return "Unknown";

    // Common patterns for AMD CPUs in HP Pavilion 15 Gaming
    // Examples:
    // "AMD Ryzen 7 3750H with Radeon Vega Mobile Gfx" -> "Ryzen 7 3750H"
    // "AMD Ryzen 5 3550H with Radeon Vega Mobile Gfx" -> "Ryzen 5 3550H"

    QStringList parts = fullName.split(' ');
    if (parts.size() >= 4) {
        // Look for "Ryzen" pattern
        for (int i = 0; i < parts.size() - 2; i++) {
            if (parts[i] == "Ryzen") {
                // Return "Ryzen X ZZZH" format
                return parts.mid(i, 3).join(" ");
            }
        }

        // Look for "AMD" followed by Ryzen
        for (int i = 0; i < parts.size() - 3; i++) {
            if (parts[i] == "AMD" && parts[i+1] == "Ryzen") {
                // Return "Ryven X ZZZH" format
                return parts.mid(i+1, 3).join(" ");
            }
        }
    }

    // If we can't parse it properly, return original but cleaned
    QString cleaned = fullName;
    cleaned.remove("with Radeon Vega Mobile Gfx");
    cleaned.remove("with Radeon Graphics");
    cleaned = cleaned.trimmed();
    return cleaned;
}

// Utility function to parse GPU name and extract just the model
QString SystemBackend::parseGpuName(const QString& fullName) const {
    if (fullName == "N/A" || fullName == "Unknown") return fullName;

    // Regular expression to match NVIDIA GPU names, optionally with "NVIDIA" prefix
    // Captures the series and model (e.g., "GTX 1650", "RTX 3060")
    // Case-insensitive.
    QRegularExpression re(QStringLiteral("(?:NVIDIA\\s+)?(GeForce|GTX|RTX)\\s+(\\S+\\s*\\d+)"),
                          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(fullName);
    if (match.hasMatch()) {
        // We have two capture groups: group 1 is the keyword (GeForce, GTX, RTX), group 2 is the rest (e.g., "GTX 1650" or "1650"?)
        // Actually, the pattern: (GeForce|GTX|RTX)\\s+(\\S+\\s*\\d+)
        // Group 1: the keyword (GeForce, GTX, RTX)
        // Group 2: the model part (which may be just the number or include the series again?).
        // We used \\S+\\s*\\d+ which matches a non-whitespace sequence, then optional whitespace, then digits.
        // This might not capture the series if it's included in the model part.
        // Let's change the pattern to capture the series and model together after the keyword.
        // We'll do: (?:NVIDIA\\s+)?(GeForce|GTX|RTX)\\s+((?:GeForce|GTX|RTX)\\s+\\d+)
        // But that would duplicate the series.
        // Instead, we can capture the entire match after the optional NVIDIA and then remove the keyword?
        // Actually, we want to return the series and model as they appear in the string (without the optional NVIDIA).
        // We can capture group 0 (the entire match) and then remove the "NVIDIA" prefix if present.
        // Let's do that.
        QString matched = match.captured(0);
        if (matched.startsWith(QStringLiteral("NVIDIA"), Qt::CaseInsensitive)) {
            matched.remove(0, 6); // remove "NVIDIA"
            matched = matched.trimmed();
        }
        return matched;
    }

    // Fallback: original logic with case-insensitivity
    QStringList parts = fullName.split(' ');
    if (parts.size() >= 3) {
        // Look for GeForce/GTX/RTX pattern (case-insensitive)
        for (int i = 0; i < parts.size() - 2; i++) {
            QString part = parts[i].toLower();
            if (part == QStringLiteral("geforce") || part == QStringLiteral("gtx") || part == QStringLiteral("rtx")) {
                // Return the series and model (e.g., "GTX 1650")
                return parts[i] + " " + parts[i+1];
            }
        }
    }

    // Fallback: return last two points if they look like a GPU model
    if (parts.size() >= 2) {
        QString lastPart = parts[parts.size() - 1];
        QString secondLast = parts[parts.size() - 2];
        // Check if last part looks like a model number (contains digits)
        if (lastPart.contains(QRegularExpression("\\d"))) {
            return secondLast + " " + lastPart;
        }
    }

    // If we can't parse it properly, return original
    return fullName;
}
