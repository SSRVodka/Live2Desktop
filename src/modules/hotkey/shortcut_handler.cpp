
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

HHOOK GlobalHotKeyHook::g_hHook = nullptr;

GlobalHotKeyHook* GlobalHotKeyHook::instance() {
    static GlobalHotKeyHook instance;
    return &instance;
}

GlobalHotKeyHook::GlobalHotKeyHook(QObject *parent) : QObject(parent) {
    installHook();
}

GlobalHotKeyHook::~GlobalHotKeyHook() {
    uninstallHook();
}

void GlobalHotKeyHook::installHook() {
    if (!g_hHook) {
        g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
        if (!g_hHook) {
            std::string msg = "SetWindowsHookEx failed:" + std::to_string(GetLastError());
            stdLogger.Exception(msg);
        } else stdLogger.Info("Registered global hotkey hook");
    }
}

void GlobalHotKeyHook::uninstallHook() {
    if (g_hHook) {
        UnhookWindowsHookEx(g_hHook);
        g_hHook = nullptr;
        stdLogger.Info("Unregistered global hotkey hook");
    }
}

LRESULT CALLBACK GlobalHotKeyHook::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    stdLogger.Debug("event");
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *kbStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
        stdLogger.Debug("pressed");

        if (wParam == WM_KEYDOWN && kbStruct->vkCode == GlobalHotKeyHandler::hotKeyChar
            && (ctrlPressed || !GlobalHotKeyHandler::useCtrl)
            && (shiftPressed || !GlobalHotKeyHandler::useShift)) {
            std::string msg = std::string("Hotkey '") + GlobalHotKeyHandler::hotKeyChar + "' pressed ("
                + std::string("with ctrl=") + std::to_string(ctrlPressed) + ", shift=" + std::to_string(shiftPressed) + ")";
            stdLogger.Info(msg);

            // 通过单例触发信号（确保在主线程）
            QMetaObject::invokeMethod(instance(), "triggerSignal", Qt::QueuedConnection);
        }
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}


bool GlobalHotKeyHandler::useCtrl = true;
bool GlobalHotKeyHandler::useShift = true;
char GlobalHotKeyHandler::hotKeyChar = 'R';

GlobalHotKeyHandler::GlobalHotKeyHandler(QObject *parent): QObject(parent) {
    this->keyHook = GlobalHotKeyHook::instance();
    connect(this->keyHook, SIGNAL(hotKeyPressed()), this, SIGNAL(hotKeyActivate()));
}

GlobalHotKeyHandler::~GlobalHotKeyHandler() {
}

#endif
