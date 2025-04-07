#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <windows.h>
static HHOOK g_hHook = nullptr;  // Handle for the keyboard hook

// Low-level keyboard hook callback function
static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

class KeyboardHookThread : public QThread {
    Q_OBJECT
public:
    KeyboardHookThread(QObject *parent = nullptr);
    void run() override;
    void stopListening();
private:
    bool stop;
};

