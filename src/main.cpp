
#include <QtCore/QDir>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include "utils/cleaner.hpp"
#include "utils/logger.h"
#include "gui/mainWindow.h"
#include "drivers/resourceLoader.h"


int main(int argc, char *argv[]) {
    /* Initialize random seed. */
    srand(time(NULL));

    QApplication app(argc, argv);
    chdir(QCoreApplication::applicationDirPath().toStdString().c_str());

    /* ------------ check app singleton START --------------- */

    QString lockFile = QCoreApplication::applicationDirPath() + "/app.lock";
    
    if (QFile::exists(lockFile)) {
        QMessageBox::critical(
            nullptr, 
            QObject::tr("Application Singleton Check"), 
            QObject::tr("The application is already starting/running and multiple instances are not allowed.\n"
                "If you are pretty sure that the application is not running, manually delete the app.lock file in the run directory.")
        );
        return 0;
    }
    QFile file(lockFile);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(
            nullptr,
            QObject::tr("Application Singleton Check"), 
            QObject::tr("Unable to create program lock file, please check file permissions")
        );
        return 0;
    }
    file.close();
    
    Cleaner::instance().register_cleanup([lockFile]() {
        QFile::remove(lockFile);
    });

    /* ------------ check app singleton END --------------- */


    if(resourceLoader::get_instance().initialize() == false) {
        stdLogger.Exception("Failed to initialize resource loader, app aborted.");
        return 0;
    }
    /* register global cleaner (singleton) for resourceLoader */
    Cleaner::instance().register_cleanup([]() {
        resourceLoader::get_instance().release();
    });

    mainWindow *win = new mainWindow(qApp);
    win->show();
    stdLogger.Info("App starts...");
    int ret = app.exec();
    if (ret) {
        stdLogger.Exception("App exited unexpectedly");
    } else {
        stdLogger.Info("App exited normally");
    }
    delete win;

    Cleaner::instance().execute_cleanup();

    return ret;
}
