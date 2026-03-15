/**
 * @file platformWindowManager.cpp
 * @brief Unified platform window manager implementation
 */

#include <QtCore/QThread>
#include <QtCore/QSettings>
#include <QtCore/QDataStream>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QApplication>

#include "utils/logger.h"
#include "utils/platformWindowManager.h"

PlatformWindowManager::PlatformWindowManager()
    : m_platform(Platform::Unknown), m_appClass("Live2Desktop") {
    m_platform = detectPlatform();
    
    switch (m_platform) {
        case Platform::Windows:
            stdLogger.Info("Platform: Windows");
            break;
        case Platform::X11:
            stdLogger.Info("Platform: X11");
            break;
        case Platform::Wayland_Hyprland:
            stdLogger.Info("Platform: Wayland (Hyprland)");
            break;
        case Platform::Wayland_Other:
            stdLogger.Info("Platform: Wayland (Other)");
            break;
        case Platform::Unknown:
        default:
            stdLogger.Warning("Platform: Unknown (maybe unsupported)");
            break;
    }
}

PlatformWindowManager::Platform PlatformWindowManager::detectPlatform() {
    // Check if running on Windows
#ifdef _WIN32
    return Platform::Windows;
#endif

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString xdgSessionType = env.value("XDG_SESSION_TYPE", "").toLower();
    QString xdgCurrentDesktop = env.value("XDG_CURRENT_DESKTOP", "").toLower();
    QString waylandDisplay = env.value("WAYLAND_DISPLAY", "");
    QString display = env.value("DISPLAY", "");

    // Check for Hyprland first
    if (xdgCurrentDesktop == "hyprland") {
        return Platform::Wayland_Hyprland;
    }

    // Check for other Wayland sessions
    if (!waylandDisplay.isEmpty() && xdgSessionType == "wayland") {
        // Try to detect if it's Hyprland
        QProcess proc;
        proc.start("hyprctl", {"-i", "version"});
        proc.waitForFinished(1000);
        if (proc.exitCode() == 0) {
            return Platform::Wayland_Hyprland;
        }
        return Platform::Wayland_Other;
    }

    // Check for X11
    if (!display.isEmpty() || xdgSessionType == "x11") {
        return Platform::X11;
    }

    return Platform::Unknown;
}

QJsonObject PlatformWindowManager::findWindow() {
    if (m_appClass.isEmpty() || m_platform != Platform::Wayland_Hyprland) {
        return QJsonObject();
    }

    QProcess proc;
    proc.start("hyprctl", {"clients", "-j"});
    proc.waitForFinished(2000);

    if (proc.exitCode() != 0) {
        stdLogger.Warning("Failed to get hyprctl clients");
        return QJsonObject();
    }

    QByteArray output = proc.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isArray()) {
        return QJsonObject();
    }

    QJsonArray clients = doc.array();
    for (const QJsonValue& client : clients) {
        QJsonObject obj = client.toObject();
        QString className = obj.value("class").toString();
        if (className == m_appClass) {
            return obj;
        }
    }

    return QJsonObject();
}

bool PlatformWindowManager::waitForWindow(int timeoutMs) {
    const int intervalMs = 100;
    int elapsed = 0;

    while (elapsed < timeoutMs) {
        if (!findWindow().isEmpty()) {
            return true;
        }
        QThread::msleep(intervalMs);
        elapsed += intervalMs;
    }

    return false;
}

bool PlatformWindowManager::executeHyprctl(const QStringList& args) {
    QProcess proc;
    proc.start("hyprctl", args);
    proc.waitForFinished(2000);

    if (proc.exitCode() != 0) {
        QString cmd = args.join(" ");
        stdLogger.Warning("hyprctl failed:" + cmd.toStdString());
        return false;
    }
    return true;
}

bool PlatformWindowManager::setFloating(bool floating) {
    if (m_platform != Platform::Wayland_Hyprland || m_appClass.isEmpty()) {
        return false;
    }

    QString action = floating ? "float" : "tiling";
    bool success = executeHyprctl({"dispatch", action + ",class:" + m_appClass});

    if (success) {
        std::string actionStr = action.toStdString();
        stdLogger.Info("Window set to" + actionStr);
    }

    return success;
}

bool PlatformWindowManager::restoreGeometry(const QPoint& pos, const QSize& size) {
    if (m_platform != Platform::Wayland_Hyprland) {
        return false;
    }

    stdLogger.Info("Restoring geometry on Hyprland: pos=("
        + std::to_string(pos.x()) + ", " + std::to_string(pos.y()) + "), size=("
        + std::to_string(size.width()) + ", " + std::to_string(size.height()) + ")");

    if (!waitForWindow(5000)) {
        stdLogger.Warning("Window not found in Hyprland, skipping geometry restore");
        return false;
    }

    bool moveSuccess = executeHyprctl({
        "dispatch", 
        QString("movewindowpixel exact %1 %2,class:%3")
            .arg(pos.x()).arg(pos.y()).arg(m_appClass)
    });

    bool resizeSuccess = executeHyprctl({
        "dispatch",
        QString("resizewindowpixel exact %1 %2,class:%3")
            .arg(size.width()).arg(size.height()).arg(m_appClass)
    });

    bool floatSuccess = setFloating(true);

    return moveSuccess && resizeSuccess && floatSuccess;
}

bool PlatformWindowManager::saveGeometry(QPoint& outPos, QSize& outSize) {
    if (m_platform == Platform::Wayland_Hyprland) {
        // On Hyprland, get actual position from compositor
        if (!waitForWindow(3000)) {
            stdLogger.Warning("Window not found in Hyprland, cannot save geometry");
            return false;
        }

        QJsonObject win = findWindow();
        if (win.isEmpty()) {
            stdLogger.Warning("Failed to find window in Hyprland");
            return false;
        }

        // Get position from Hyprland (at is an array [x, y])
        QJsonArray at = win.value("at").toArray();
        int x = at[0].toInt(0);
        int y = at[1].toInt(0);
        outPos = QPoint(x, y);

        // Get size from Hyprland (size is an array [width, height])
        QJsonArray sizeArr = win.value("size").toArray();
        int w = sizeArr[0].toInt(0);
        int h = sizeArr[1].toInt(0);
        outSize = QSize(w, h);

        stdLogger.Info("Saved geometry from Hyprland: pos=("
            + std::to_string(outPos.x()) + ", " + std::to_string(outPos.y()) + "), size=("
            + std::to_string(outSize.width()) + ", " + std::to_string(outSize.height()) + ")");
        return true;
    }

    // For other platforms, return false to indicate Qt should handle it
    return false;
}

Qt::WindowFlags PlatformWindowManager::getNormalWindowFlags() const {
    if (m_platform == Platform::Wayland_Hyprland) {
        return Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool;
    } else {
        return Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint;
    }
}

Qt::WindowFlags PlatformWindowManager::getGeometryEditWindowFlags() const {
    if (m_platform == Platform::Wayland_Hyprland) {
        return Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool;
    } else {
        return Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint;
    }
}

void PlatformWindowManager::applyWindowFlags(QWidget* widget, Qt::WindowFlags flags) {
    if (widget) {
        widget->setWindowFlags(flags);
        widget->setWindowTitle("Geometry Edit (Enter to confirm)");
    }
}

void PlatformWindowManager::applyGeometryEditModeFlags(QWidget* widget) {
    if (!widget) {
        return;
    }

    if (m_platform == Platform::Wayland_Hyprland) {
        setFloating(true);
    } else {
        // For X11/Windows: Show window frame for manual resize
        Qt::WindowFlags flags = Qt::WindowStaysOnTopHint;
        flags &= ~Qt::WindowMaximizeButtonHint;
        flags &= ~Qt::WindowCloseButtonHint;
        widget->setWindowFlags(flags);
    }
    widget->show();
}

void PlatformWindowManager::restoreNormalModeFlags(QWidget* widget) {
    if (!widget) {
        return;
    }

    if (m_platform == Platform::Wayland_Hyprland) {
        setFloating(true);
    } else {
        // For X11/Windows: Restore original frameless mode
        Qt::WindowFlags flags = getNormalWindowFlags();
        flags &= ~Qt::WindowMaximizeButtonHint;
        flags &= ~Qt::WindowCloseButtonHint;
        widget->setWindowFlags(flags);
    }

#ifdef _WIN32
    ::SetWindowPos(HWND(widget->winId()), HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    ::SetWindowPos(HWND(widget->winId()), HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif

    widget->show();
}

void PlatformWindowManager::saveGeometryToSettings(const QString& org, const QString& app, const QString& key, QWidget* widget) {
    QSettings settings(org, app);

    // Use platform-specific geometry saving
    QPoint pos;
    QSize size;
    if (isHyprland()) {
        // On Hyprland, get actual position from compositor
        if (saveGeometry(pos, size)) {
            settings.setValue(key + "/pos", pos);
            settings.setValue(key + "/size", size);
        }
    } else {
        // On Windows/X11, use Qt's native saveGeometry
        settings.setValue(key, widget->saveGeometry());
    }
}

bool PlatformWindowManager::restoreGeometryFromSettings(const QString& org, const QString& app, const QString& key, QWidget* widget) {
    if (!widget) {
        return false;
    }

    QSettings settings(org, app);

    if (m_platform == Platform::Wayland_Hyprland) {
        // Hyprland: read from separate pos/size values
        QVariant posVar = settings.value(key + "/pos");
        QVariant sizeVar = settings.value(key + "/size");

        QPoint pos;
        QSize size;
        bool useHyprctl = false;

        if (posVar.isValid() && sizeVar.isValid()) {
            // Use saved position/size from settings
            pos = posVar.value<QPoint>();
            size = sizeVar.value<QSize>();
        } else {
            // Fallback: get from hyprctl if no saved settings
            stdLogger.Info("No saved geometry found in settings, fetching from Hyprland");
            useHyprctl = true;
        }

        if (useHyprctl) {
            // Fetch current position/size from Hyprland
            if (!saveGeometry(pos, size)) {
                stdLogger.Warning("Failed to fetch geometry from Hyprland");
                return false;
            }
        }

        return restoreGeometry(pos, size);
    } else {
        // X11/Windows: read from QByteArray
        QByteArray geom = settings.value(key).toByteArray();
        if (geom.isEmpty()) {
            return false;
        }
        widget->restoreGeometry(geom);
        return true;
    }
}
