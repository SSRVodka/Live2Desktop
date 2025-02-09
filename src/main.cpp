#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include <QtCore/QDir>
#include <QtWidgets/QApplication>

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
    mainWindow win(qApp);
    win.show();
    stdLogger.Info("App starts...");
    return app.exec();
}
