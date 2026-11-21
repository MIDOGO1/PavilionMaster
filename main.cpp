#include <QDebug>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QQmlError>
#include <QQmlEngine>
#include <QWindow>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <windows.h>

// DWM rounded corners support (missing from older MinGW headers)
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
typedef enum {
    DWMWCP_DEFAULT    = 0,
    DWMWCP_DONOTROUND = 1,
    DWMWCP_ROUND      = 2,
    DWMWCP_ROUNDSMALL = 3
} DWM_WINDOW_CORNER_PREFERENCE;

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include "SystemBackend.h"
#include "SingleInstance.h"

// Message handler to log Qt messages to a file
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QString txt = QString("[%1] %2")
                      .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                      .arg(localMsg.constData());
    QFile outFile(QCoreApplication::applicationDirPath() + "/control_center.log");
    if (!outFile.open(QIODevice::Append | QIODevice::Text))
        return;
    QTextStream ts(&outFile);
    ts << txt << "\n";
    outFile.close();

    // Also output to debugger
    fprintf(stderr, "%s\n", txt.toLocal8Bit().constData());
    fflush(stderr);
}

// Registers the fonts embedded from the ./fonts folder (see CMakeLists.txt) so the UI looks
// the same on PCs that don't have them installed. Must run before the QML is loaded.
static void loadBundledFonts()
{
    const QString dirPath = QStringLiteral(":/qt/qml/GControlCetner/fonts");
    const QStringList files = QDir(dirPath).entryList(QDir::Files);

    if (files.isEmpty())
        qWarning() << "No fonts are bundled in the executable - text will use whatever is installed on this PC";

    for (const QString &file : files) {
        const int id = QFontDatabase::addApplicationFont(dirPath + "/" + file);
        if (id < 0)
            qWarning() << "Failed to load bundled font:" << file;
        else
            qInfo() << "Loaded bundled font:" << file << QFontDatabase::applicationFontFamilies(id);
    }

    // Some Inter downloads register their static fonts as "Inter 18pt" instead of "Inter".
    // Map the name the QML uses ("Inter") onto whichever one was actually loaded.
    if (!QFontDatabase::families().contains("Inter")) {
        for (const QString &family : QFontDatabase::families()) {
            if (family.startsWith("Inter")) {
                qInfo() << "Using font family" << family << "for \"Inter\"";
                QFont::insertSubstitution("Inter", family);
                break;
            }
        }
    }

    // Only used when a family is neither bundled nor installed: keeps the layout sane
    QFont::insertSubstitution("Inter",  "Segoe UI");
    QFont::insertSubstitution("AKONY",  "Segoe UI Black");
    QFont::insertSubstitution("Angled", "Segoe UI Semibold");
}

int main(int argc, char *argv[]) {
    // Only one Pavilion Master at a time: a second launch just raises the running window
    if (!acquireSingleInstance(L"HP Pavilion 15 Gaming Control Center"))
        return 0;

    QApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);

    qInfo() << "Control Center application starting";
    loadBundledFonts();

    QQmlApplicationEngine engine;

    SystemBackend backend;
    engine.rootContext()->setContextProperty("backend", &backend);
    qInfo() << "SystemBackend created";
    // Never abort here: on unsupported hardware the backend runs in a limited mode and the
    // QML shows the "Unsupported Processor" screen.
    if (backend.initializationFailed())
        qWarning() << "Backend reported an initialization problem - continuing in limited mode";

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() {
                         qCritical() << "QML FAILED TO LOAD (objectCreationFailed)";
                         QMessageBox::critical(nullptr, "Control Center Error",
                                             "Failed to load QML module. See log file for details.");
                         QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);

    engine.loadFromModule("GControlCetner", "Frame_1");

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "NO ROOT OBJECTS - QML DID NOT LOAD";
        QMessageBox::critical(nullptr, "Control Center Error",
                              "QML module loaded but no root objects created. See log file for details.");
        return -1;
    }

    qInfo() << "QML loaded successfully, showing window";

    // Disable DWM corner rounding (the QML card draws its own rounded shape)
    auto roots = engine.rootObjects();
    if (!roots.isEmpty()) {
        QWindow* win = qobject_cast<QWindow*>(roots.first());
        if (win) {
            HWND hwnd = (HWND)win->winId();
            DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_DONOTROUND;
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                                  &pref, sizeof(pref));
        }
    }

    qInfo() << "Entering application event loop";
    return app.exec();
}
