/**
 * @file shortcut_handler.h
 * @brief global hotkey handler class
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>

#include "utils/logger.h"

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/keysym.h>
// #include "modules/hotkey/platform/hotkey_linux.h"
class GlobalKeyListener;
#endif

#ifdef Q_OS_WIN
#include <windows.h>
// #include "modules/hotkey/platform/hotkey_win.h"
class GlobalHotKeyHook;
#endif


/**
 * @brief Encapsulation of platform-related implementation for hot key handler
 * @note modify static members ONLY before initializing instances
 */
class GlobalHotKeyHandler: public QObject {
    Q_OBJECT
public:

    #ifdef Q_OS_LINUX
    static KeySym hotkey;               // default XK_R
    static unsigned int x_modifiers;    // default ControlMask | ShiftMask
    #endif

    #ifdef Q_OS_WIN
    static bool useCtrl;        // default true
    static bool useShift;       // default true
    static char hotKeyChar;     // default 'R'
    #endif

    GlobalHotKeyHandler(QObject *parent = nullptr);
    ~GlobalHotKeyHandler();

signals:
    void hotKeyActivate();
private:
    #ifdef Q_OS_LINUX
    Display *display;
    GlobalKeyListener *listener;
    #endif

    #ifdef Q_OS_WIN
    GlobalHotKeyHook *keyHook;
    #endif
};
