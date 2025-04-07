
#include <QtCore/QCoreApplication>

#include "modules/hotkey/shortcut_handler.h"


#ifdef Q_OS_LINUX
#include "modules/hotkey/platform/hotkey_linux.h"

GlobalKeyListener::GlobalKeyListener(
    KeySym key, unsigned int mod,
    Display *display, QObject *parent)
    : QThread(parent), hotkey(key), xmask(mod), display(display), stop(false) {}


void GlobalKeyListener::run() {
    // Get the root window
    Window root = DefaultRootWindow(display);

    KeyCode keyCode = XKeysymToKeycode(display, hotkey);
    unsigned int modifiers = xmask;

    // Grab the key combination globally
    XGrabKey(display, keyCode, modifiers, root, True, GrabModeAsync, GrabModeAsync);
    XSelectInput(display, root, KeyPressMask);
    std::string origm = "Registered global key event: modifiers = "
        + std::to_string(modifiers) + ", key = " + std::to_string(keyCode);
    stdLogger.Info(origm.c_str());
    
    std::string msg = "Global hot key pressed: X11 modifiers = "
        + std::to_string(modifiers) + ", X11 key code = " + std::to_string(keyCode);

    // Event loop
    while (!stop) {
        XEvent event;
        if (XPending(display) > 0) {
            XNextEvent(display, &event);

            if (event.type == KeyPress) {
                stdLogger.Debug(msg.c_str());
                emit keyPressed();  // Emit signal when key is pressed
            }
        }
        QThread::msleep(100);  // Slight delay to prevent high CPU usage
    }

    // Clean up
    XUngrabKey(display, keyCode, modifiers, root);
}

void GlobalKeyListener::stopListening() {
    stop = true;
}


KeySym GlobalHotKeyHandler::hotkey = XK_R;
unsigned int GlobalHotKeyHandler::x_modifiers = ControlMask | ShiftMask;

GlobalHotKeyHandler::GlobalHotKeyHandler(QObject *parent): QObject(parent) {
    // Initialize X11 display
    display = XOpenDisplay(nullptr);
    if (!display) {
        stdLogger.Exception("Unable to open X display");
        QCoreApplication::exit(1);
    }

    // Start the global key listener thread
    listener = new GlobalKeyListener(hotkey, x_modifiers, display, this);
    connect(listener, &GlobalKeyListener::keyPressed, this, &GlobalHotKeyHandler::hotKeyActivate);
    listener->start();
}

GlobalHotKeyHandler::~GlobalHotKeyHandler() {
    listener->stopListening();
    listener->wait();
    delete listener;

    if (display) {
        XCloseDisplay(display);
    }
}

#endif

#ifdef Q_OS_WIN
#include "modules/hotkey/platform/hotkey_win.h"

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
        bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;

        bool modValid = (!GlobalHotKeyHandler::useCtrl || ctrlPressed)
                    && (!GlobalHotKeyHandler::useShift || shiftPressed);

        // Check for the key combination: Ctrl + Shift + R
        if (modValid && wParam == WM_KEYDOWN && pKeyBoard->vkCode == GlobalHotKeyHandler::hotKeyChar) {
            qDebug() << "Global key combination Ctrl+Shift+R pressed!";
            // TODO
            toggleRecording();
        }
    }

    // Pass the event to the next hook in the chain
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

KeyboardHookThread::KeyboardHookThread(QObject *parent): QThread(parent), stop(false) {}

void KeyboardHookThread::run() {
    // Install the low-level keyboard hook
    g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    if (!g_hHook) {
        qFatal("Failed to install keyboard hook!");
        return;
    }

    // Message loop to keep the hook active
    MSG msg;
    while (!stop && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Uninstall the hook when the loop ends
    if (g_hHook) {
        UnhookWindowsHookEx(g_hHook);
        g_hHook = nullptr;
    }
}

void KeyboardHookThread::stopListening() {
    stop = true;
    PostThreadMessage(GetCurrentThreadId(), WM_QUIT, 0, 0); // Exit the message loop
}

bool GlobalHotKeyHandler::useCtrl = true;
bool GlobalHotKeyHandler::useShift = true;
char GlobalHotKeyHandler::hotKeyChar = 'R';

GlobalHotKeyHandler::GlobalHotKeyHandler(QObject *parent): QObject(parent) {
    // Start the keyboard hook thread
    hookThread = new KeyboardHookThread(this);
    hookThread->start();
}

GlobalHotKeyHandler::~GlobalHotKeyHandler() {
    // Stop the hook thread
    hookThread->stopListening();
    hookThread->wait();
    delete hookThread;
}

#endif
