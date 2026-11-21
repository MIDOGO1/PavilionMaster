#pragma once
// One Windows power plan per Pavilion Master mode:
//   Quiet       -> Power saver
//   Balanced    -> Balanced
//   Performance -> "Pavilion Master Ultimate Performance" (created on first use)
#include <QString>
#include <QStringList>
#include <QProcess>

namespace PowerPlans {

// Built-in Windows plan GUIDs
inline const QString kPowerSaver  = QStringLiteral("a1841308-3541-4fab-bc81-f71556f20b4a");
inline const QString kBalanced    = QStringLiteral("381b4222-f694-41f0-9685-ff5bb260df2e");
inline const QString kHighPerf    = QStringLiteral("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c");
inline const QString kUltimateSrc = QStringLiteral("e9a42b02-d5df-448d-aa00-03f14749eb61");

// Our own fixed GUID so we can find/reuse/delete the plan later
inline const QString kPmUltimate  = QStringLiteral("5a7c1e2b-9d34-4f6a-8b1c-0e7d2f4a6b93");
inline const QString kPmName      = QStringLiteral("Pavilion Master Ultimate Performance");
inline const QString kPmDesc      = QStringLiteral("Created by Pavilion Master");

inline bool powercfg(const QStringList &args, QString *out = nullptr)
{
    QProcess p;
    p.start(QStringLiteral("powercfg.exe"), args);
    if (!p.waitForFinished(5000)) { p.kill(); return false; }
    if (out) *out = QString::fromLocal8Bit(p.readAllStandardOutput());
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

// Activate a plan; if the plan was removed from this PC (some OEM images drop
// Power saver), restore it from its default and try again.
inline bool activate(const QString &guid)
{
    if (powercfg({QStringLiteral("/setactive"), guid})) return true;
    powercfg({QStringLiteral("/duplicatescheme"), guid});   // restores a missing built-in plan
    return powercfg({QStringLiteral("/setactive"), guid});
}

// Creates "Pavilion Master Ultimate Performance" if needed and returns its GUID.
// Falls back to a duplicate of High performance where Ultimate isn't offered.
inline QString ensureUltimate()
{
    QString list;
    powercfg({QStringLiteral("/list")}, &list);
    if (list.contains(kPmUltimate, Qt::CaseInsensitive))
        return kPmUltimate;

    QString name = kPmName;
    bool ok = powercfg({QStringLiteral("/duplicatescheme"), kUltimateSrc, kPmUltimate});
    if (!ok) {
        ok = powercfg({QStringLiteral("/duplicatescheme"), kHighPerf, kPmUltimate});
        name = QStringLiteral("Pavilion Master Performance");
    }
    if (!ok) return kHighPerf;   // last resort: built-in High performance

    powercfg({QStringLiteral("/changename"), kPmUltimate, name, kPmDesc});
    return kPmUltimate;
}

inline bool applyQuiet()       { return activate(kPowerSaver); }
inline bool applyBalanced()    { return activate(kBalanced);   }
inline bool applyPerformance() { return activate(ensureUltimate()); }

} // namespace PowerPlans
