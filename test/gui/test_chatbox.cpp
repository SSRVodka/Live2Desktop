
#include "gui/chatBox.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ChatBox box(nullptr);
    return box.exec();
}
