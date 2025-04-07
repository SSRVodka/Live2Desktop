
#include <QtWidgets/QApplication>

#include "modules/hotkey/shortcut_handler.h"

#include "utils/logger.h"


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    GlobalHotKeyHandler handler;

    return app.exec();
}
