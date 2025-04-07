#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <windows.h>

class GlobalHotKeyHook : public QObject {
    Q_OBJECT
public:
    static GlobalHotKeyHook* instance();
    void installHook();
    void uninstallHook();

private slots:
    void triggerSignal() { emit hotKeyPressed(); }

signals:
    void hotKeyPressed();

private:
    GlobalHotKeyHook(QObject *parent = nullptr);
    ~GlobalHotKeyHook();

    static HHOOK g_hHook;
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
};

