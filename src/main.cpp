
#include <QtCore/QDir>
#include <QtWidgets/QApplication>

#include "utils/cleaner.hpp"
#include "utils/logger.h"
#include "gui/mainWindow.h"
#include "drivers/resourceLoader.h"


int main(int argc, char *argv[]) {
    /* Initialize random seed. */
    srand(time(NULL));

    QApplication app(argc, argv);
    chdir(QCoreApplication::applicationDirPath().toStdString().c_str());
    if(resourceLoader::get_instance().initialize() == false) {
        stdLogger.Exception("Failed to initialize resource loader, app aborted.");
        return 0;
    }
    /* register global cleaner (singleton) for resourceLoader */
    Cleaner::instance().register_cleanup([]() {
        resourceLoader::get_instance().release();
    });

    mainWindow win(qApp);
    win.show();
    stdLogger.Info("App starts...");
    int ret = app.exec();
    if (ret) {
        stdLogger.Exception("App exited unexpectedly");
    } else {
        stdLogger.Info("App exited normally");
    }

    return ret;
}
