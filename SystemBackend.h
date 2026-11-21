#pragma once
#include <QObject>
#include <QString>
#include <QTimer>
#include "FanControl.h"
#include "nvml.h"   // types only - the NVML library itself is loaded at runtime (see initNvml)

// NVML entry points, resolved with GetProcAddress. Loading at runtime (instead of linking
// nvml.lib) means the app still starts on PCs with no NVIDIA driver / GPU.
struct NvmlApi {
    void* lib = nullptr;   // HMODULE
    nvmlReturn_t (*init)() = nullptr;
    nvmlReturn_t (*getHandle)(unsigned int, nvmlDevice_t*) = nullptr;
    nvmlReturn_t (*getName)(nvmlDevice_t, char*, unsigned int) = nullptr;
    nvmlReturn_t (*getTemperature)(nvmlDevice_t, nvmlTemperatureSensors_t, unsigned int*) = nullptr;
    nvmlReturn_t (*getPowerUsage)(nvmlDevice_t, unsigned int*) = nullptr;
};

// One RyzenADJ power profile. Watts are converted to mW when the command is built.
struct PowerProfile {
    int stapmW;   // --stapm-limit  (sustained power)
    int fastW;    // --fast-limit   (short boost PPT)
    int slowW;    // --slow-limit   (average PPT)
    int tctlC;    // --tctl-temp    (temperature limit, degrees C)
};

// A supported processor and its three profiles
struct CpuSpec {
    const char* model;       // matched against the CPU name, e.g. "5800H"
    const char* codename;
    PowerProfile performance;
    PowerProfile balanced;
    PowerProfile quiet;
};

class SystemBackend : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString cpuTemp    READ cpuTemp    NOTIFY dataChanged)
    Q_PROPERTY(QString cpuWatts   READ cpuWatts   NOTIFY dataChanged)
    Q_PROPERTY(QString dgpuTemp   READ dgpuTemp   NOTIFY dataChanged)
    Q_PROPERTY(QString dgpuWatts  READ dgpuWatts  NOTIFY dataChanged)
    Q_PROPERTY(QString igpuTemp   READ igpuTemp   NOTIFY dataChanged)
    Q_PROPERTY(QString igpuWatts  READ igpuWatts  NOTIFY dataChanged)
    Q_PROPERTY(QString totalWatts READ totalWatts NOTIFY dataChanged)
    Q_PROPERTY(QString battCharge READ battCharge NOTIFY dataChanged)
    Q_PROPERTY(QString battHealth READ battHealth NOTIFY dataChanged)
    Q_PROPERTY(QString fanMode    READ fanMode    NOTIFY dataChanged)
    Q_PROPERTY(QString perfMode   READ perfMode   NOTIFY dataChanged)
    Q_PROPERTY(QString cpuName    READ cpuName    CONSTANT)
    Q_PROPERTY(QString gpuName    READ gpuName    CONSTANT)
    Q_PROPERTY(QString ramSize    READ ramSize    CONSTANT)
    Q_PROPERTY(QString moboName   READ moboName   CONSTANT)
    Q_PROPERTY(QString cpuNameParsed READ cpuNameParsed CONSTANT)
    Q_PROPERTY(QString gpuNameParsed READ gpuNameParsed CONSTANT)
    // True when the detected CPU is one of the supported models (decides whether the controls are enabled)
    Q_PROPERTY(bool cpuSupported READ cpuSupported CONSTANT)
    // Saved laptop color variant ("" until the user picks one on first launch)
    Q_PROPERTY(QString accentVariant READ accentVariant NOTIFY accentVariantChanged)

public:
    explicit SystemBackend(QObject* parent = nullptr);
    ~SystemBackend();

    QString cpuTemp()    const { return m_cpuTemp; }
    QString cpuWatts()   const { return m_cpuWatts; }
    QString dgpuTemp()   const { return m_dgpuTemp; }
    QString dgpuWatts()  const { return m_dgpuWatts; }
    QString igpuTemp()   const { return m_igpuTemp; }
    QString igpuWatts()  const { return m_igpuWatts; }
    QString totalWatts()  const { return m_totalWatts; }
    QString battCharge() const { return m_battCharge; }
    QString battHealth() const { return m_battHealth; }
    QString fanMode()    const { return m_fanMode; }
    QString perfMode()   const { return m_perfMode; }
    QString cpuName()    const { return m_cpuName; }
    QString gpuName()    const { return m_gpuName; }
    QString ramSize()    const { return m_ramSize; }
    QString moboName()   const { return m_moboName; }
    QString cpuNameParsed() const { return parseCpuName(m_cpuName); }
    QString gpuNameParsed() const { return parseGpuName(m_gpuName); }
    bool cpuSupported() const { return m_cpuSpec != nullptr; }
    QString accentVariant() const { return m_accentVariant; }

    Q_INVOKABLE void setPerformanceMode();
    Q_INVOKABLE void setBalancedMode();
    Q_INVOKABLE void setQuietMode();
    Q_INVOKABLE void toggleFan();
    Q_INVOKABLE void saveAccentVariant(const QString& key);
    bool initializationFailed() const { return m_initializationFailed; }
    bool fanAvailable() const { return m_fanAvailable; }

signals:
    void dataChanged();
    void accentVariantChanged();

private slots:
    void refresh();

private:
    FanControl m_fan;
    NvmlApi m_nvml;
    nvmlDevice_t m_gpu = nullptr;
    QTimer* m_timer;
    bool m_nvmlReady = false;
    bool m_fanAvailable = false;

    QString m_battHealth  = "Calculating...";
    QString m_cpuTemp     = "CPU: --°C";
    QString m_cpuWatts    = "--W";
    QString m_dgpuTemp    = "D-GPU: --°C";
    QString m_dgpuWatts   = "--W";
    QString m_igpuTemp    = "iGPU: --°C";
    QString m_igpuWatts   = "--W";
    QString m_totalWatts  = "--W";
    QString m_battCharge  = "--%";
    QString m_fanMode     = "AUTO";
    QString m_perfMode    = "NONE";
    QString m_cpuName     = "Unknown";
    QString m_gpuName     = "Unknown";
    QString m_ramSize     = "Unknown";
    QString m_moboName    = "Unknown";
    bool m_initializationFailed = false;
    QString m_accentVariant;
    const CpuSpec* m_cpuSpec = nullptr;   // nullptr = unsupported CPU

    QString parseCpuName(const QString& fullName) const;
    QString parseGpuName(const QString& fullName) const;
    bool initNvml();
    const CpuSpec* findCpuSpec(const QString& fullName) const;
    void applyProfile(const PowerProfile& profile, const QString& modeName);

    void readBatteryHealth();
    void readStaticSystemInfo();
};