#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <X11/Xlib.h>
#include <X11/keysym.h>

class GlobalKeyListener : public QThread {
    Q_OBJECT
public:
    GlobalKeyListener(
        KeySym key, unsigned int modifiers,
        Display *display, QObject *parent = nullptr);
    void run() override;
    void stopListening();
signals:
    void keyPressed();
private:
    KeySym hotkey;
    unsigned int xmask;

    Display *display;
    bool stop;
};

