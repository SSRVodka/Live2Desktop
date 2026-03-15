/**
 * @file platformWindowManager.h
 * @brief Unified platform window manager for cross-platform geometry handling
 * 
 * Supports:
 * - Windows (X11 bypass window manager)
 * - Linux X11 (X11 bypass window manager)
 * - Wayland/Hyprland (compositor IPC)
 */

#pragma once

#include <QtCore/QString>
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/QProcess>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtWidgets/QWidget>

class PlatformWindowManager {
public:
    static PlatformWindowManager& instance() {
        static PlatformWindowManager instance;
        return instance;
    }

    enum class Platform {
        Windows,
        X11,
        Wayland_Hyprland,
        Wayland_Other,
        Unknown
    };

    Platform getPlatform() const { return m_platform; }
    bool isHyprland() const { return m_platform == Platform::Wayland_Hyprland; }
    bool isX11OrWindows() const { return m_platform == Platform::X11 || m_platform == Platform::Windows; }
    QString getAppClass() const { return m_appClass; }

    void setAppClass(const QString& appClass) { m_appClass = appClass; }

    // Window flags for different modes
    Qt::WindowFlags getNormalWindowFlags() const;
    Qt::WindowFlags getGeometryEditWindowFlags() const;
    void applyWindowFlags(QWidget* widget, Qt::WindowFlags flags);
    void applyGeometryEditModeFlags(QWidget* widget);
    void restoreNormalModeFlags(QWidget* widget);

    // Geometry operations
    bool restoreGeometry(const QPoint& pos, const QSize& size);
    bool saveGeometry(QPoint& outPos, QSize& outSize);
    bool setFloating(bool floating);

    // Settings-based geometry (for Hyprland)
    void saveGeometryToSettings(const QString& org, const QString& app, const QString& key, QWidget* widget);
    bool restoreGeometryFromSettings(const QString& org, const QString& app, const QString& key, QWidget* widget);

private:
    explicit PlatformWindowManager();
    ~PlatformWindowManager() = default;

    Platform detectPlatform();
    bool waitForWindow(int timeoutMs = 5000);
    QJsonObject findWindow();
    bool executeHyprctl(const QStringList& args);

    Platform m_platform;
    QString m_appClass;
};

