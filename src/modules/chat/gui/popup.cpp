
#include "utils/consts.h"
#include "modules/chat/gui/popup.h"

Popup::Popup(const QString& showString, QWidget* parent)
  : QWidget(parent)
{
    setupUi(this);
    // setWindowFlags(
    //     Qt::Window
    //   | Qt::FramelessWindowHint
    //   | Qt::WindowStaysOnTopHint
    //   | Qt::Tool
    //   | Qt::X11BypassWindowManagerHint
    // );
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    showStr = showString;
    
    setWindowOpacity(maxOpacity);

    setText(showStr);
}

Popup::~Popup() {
    
}

