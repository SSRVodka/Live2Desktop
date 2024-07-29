#include <QtCore/QSettings>

#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtGui/QMouseEvent>

#include "configDialog.h"
#include "consts.h"
#include "logger.h"
#include "mainWindow.h"
#include "modelManager.h"
#include "resourceLoader.h"

#define PREPARE_FOR_POPUP setWindowFlags(windowFlags() & ~Qt::Tool); show();
#define HIDE_POPUP setWindowFlags(windowFlags() | Qt::Tool); show();


mainWindow::mainWindow(QApplication* mapp)
    : QMainWindow(nullptr), app(mapp),
      systemTray(new QSystemTrayIcon(this)) {
    
    setupUi(this);

    if (app == nullptr)
		app = qApp;
    
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(
        Qt::FramelessWindowHint
      | Qt::WindowStaysOnTopHint
      | Qt::Tool
      | Qt::X11BypassWindowManagerHint
    );
    setWindowTitle(
        "Geometry Edit (Enter to confirm)"
    );

    setupTray();

    loadSettings();
    loadStyleSheet();
}

mainWindow::~mainWindow() {
    writeSettings();
    stdLogger.Debug("Geometry configurations saved.");

    resourceLoader::get_instance().release();
    stdLogger.Info("App exited normally.");
}

void mainWindow::loadModels() {
    auto ms = resourceLoader::get_instance().getModelList();
    auto cm = resourceLoader::get_instance().getCurrentModelName();
    bool find = true;
    for(uint32_t i = 0; i < ms.length(); i++) {
        QAction* tmpModel = new QAction(ms[i], switchActionGroup);
        tmpModel->setCheckable(true);
        if(find && cm.compare(ms[i]) == 0) {
            tmpModel->setChecked(true);
            find = false;
        }
        modelList.push_back(std::move(tmpModel));
    }
}

void mainWindow::setupTray() {
    mainMenu = new QMenu(this);
    switchMenu = new QMenu(this);
    switchMenu->setTitle(tr("Switch"));
    switchActionGroup = new QActionGroup(switchMenu);

    loadModels();

    switchActionGroup->setExclusive(true);
    switchMenu->addActions(modelList);
    connect(switchActionGroup, &QActionGroup::triggered, this, &mainWindow::switchClicked);

    exitAction = new QAction(mainMenu);
    exitAction->setText(tr("Exit"));
    connect(exitAction, &QAction::triggered, this, &mainWindow::exitApp);

    geoEditAction = new QAction(mainMenu);
    geoEditAction->setText(tr("Geometry Edit"));
    geoEditAction->setCheckable(true);
    geoEditAction->setChecked(false);
    connect(geoEditAction, &QAction::toggled, this, &mainWindow::geoEditMode);

    settingsAction = new QAction(mainMenu);
    settingsAction->setText(tr("Settings"));
    connect(settingsAction, &QAction::triggered, this, &mainWindow::config);

    chatAction = new QAction(mainMenu);
    chatAction->setText(tr("Chat"));
    connect(chatAction, &QAction::triggered, this, &mainWindow::chatBegin);

    aboutAction = new QAction(mainMenu);
    aboutAction->setText(tr("About..."));
    connect(aboutAction, &QAction::triggered, this, &mainWindow::aboutAuthor);

    mainMenu->addAction(geoEditAction);
    mainMenu->addAction(settingsAction);
    mainMenu->addAction(chatAction);
    mainMenu->addMenu(switchMenu);
    mainMenu->addSeparator();
    mainMenu->addAction(aboutAction);
    mainMenu->addAction(exitAction);

    systemTray->setIcon(QIcon(resourceLoader::get_instance().getTrayIconPath()));
    systemTray->setToolTip(appName);
    systemTray->show();
    systemTray->setContextMenu(mainMenu);
}

void mainWindow::exitApp() {
    resourceLoader::get_instance().release();
    app->exit(0);
}

void mainWindow::switchClicked(QAction* curAct) {
    int counter = 0;
    for(QAction *&i: modelList) {
        if(i == curAct)
            break;
        counter++;
    }

    if (!resourceLoader::get_instance().setCurrentModel(counter)) {
        stdLogger.Exception("Failed to change current model (unknown error).");
        return;
    }

    QString m = resourceLoader::get_instance().getCurrentModelName();

    if (
        ModelManager::GetInstance()->ChangeScene(
            (Csm::csmChar*)m.toStdString().c_str()
        )
    ) { return; }

    /* Failed to switch to selected model, try other model(s). */
    stdLogger.Warning(
        QString("Failed to switch to selected model: %1, try other model(s).")
        .arg(m)
        .toStdString().c_str()
    );

    int _counter = 0;
    for(const QString &item: resourceLoader::get_instance().getModelList()) {
        if (_counter == counter) { ++_counter; continue; }
        stdLogger.Warning(
            QString("Try model: %1").arg(item)
            .toStdString().c_str()
        );
        if(
            ModelManager::GetInstance()->ChangeScene(
                (Csm::csmChar*)item.toStdString().c_str()
            )
        ) {
            QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon(2);
            this->systemTray->showMessage(
                appName,
                tr("Failed to switch to selected model. Load random available model instead."),
                msgIcon, 5000
            );
            this->modelList[_counter]->setChecked(true);
            resourceLoader::get_instance().setCurrentModel(_counter);
            return;
        }
        _counter++;
    }

    QMessageBox::critical(
        this, appName,
        tr("Fatal resource error (unknown).")
    );
    stdLogger.Exception("No model available, app aborted.");
    resourceLoader::get_instance().release();
    this->app->exit(0);
}

void mainWindow::geoEditMode(bool enter) {
    if (enter) {
        setWindowFlags(Qt::WindowStaysOnTopHint);
        setWindowFlags(
            windowFlags()
          & ~Qt::WindowMaximizeButtonHint
          & ~Qt::WindowCloseButtonHint
        );
    }
    else {
        setWindowFlags(
            Qt::FramelessWindowHint
          | Qt::WindowStaysOnTopHint
          | Qt::Tool
          | Qt::X11BypassWindowManagerHint
        );
        setWindowFlags(
            windowFlags()
          & ~Qt::WindowMaximizeButtonHint
          & ~Qt::WindowCloseButtonHint
        );
    }
#ifdef _WIN32
    ::SetWindowPos(HWND(this->winId()),HWND_TOPMOST,0,0,0,0,
    SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    ::SetWindowPos(HWND(this->winId()),HWND_NOTOPMOST,0,0,0,0,
    SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
#endif
    show();
}

void mainWindow::config() {
    PREPARE_FOR_POPUP;
    ConfigDialog dialog(0);
    dialog.setCurrentModel(
        resourceLoader::get_instance().getCurrentModelName()
    );
    int res = dialog.exec();
    HIDE_POPUP;
}

void mainWindow::chatBegin() {
    // TODO
    PREPARE_FOR_POPUP;
    QMessageBox::information(0, appName, "Sorry, under maintenance.");
    HIDE_POPUP;
}

void mainWindow::keyPressEvent(QKeyEvent *event) {
    if (geoEditAction->isChecked()) {
        switch (event->key()) {
        case Qt::Key_Return:
            stdLogger.Debug("Geometry configurations confirmed");
            geoEditAction->setChecked(false);
        }
    }
}

void mainWindow::closeEvent(QCloseEvent* e) {
    resourceLoader::get_instance().release();
    stdLogger.Info("App exited normally.");
    this->app->exit(0);
}

void mainWindow::writeSettings() {
    QSettings settings("SSRVodka Inc.", appName);
    settings.setValue("geometry", saveGeometry());
}

void mainWindow::loadSettings() {
    QSettings settings("SSRVodka Inc.", appName);
    restoreGeometry(settings.value("geometry").toByteArray());
}

void mainWindow::loadStyleSheet() {
    QFile file(":/style.qss");
	if(file.open(QFile::ReadOnly)){
		QString styleSheet = QLatin1String(file.readAll());
    	this->setStyleSheet(styleSheet);
    	file.close();
	} else {
        QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon(2);
        this->systemTray->showMessage(
            appName,
            tr("Failed to load page style sheet."),
            msgIcon, 5000
        );
    }
}

void mainWindow::aboutAuthor() {
    PREPARE_FOR_POPUP;
    QMessageBox::about(0, tr("About me & my program"),
    QString("<h2>%1</h2>"
        "<p>Copyright &copy; 2023 SSRVodka Inc. "
        "%1 is a small application that "
        "demonstrates numerous Qt classes, "
        "which is written by C++/Qt.</p>"
        "If you want to learn more about the application, or report a bug, "
        "please visit <a href='https://sjtuxhw.top/myBlog/'>my blog</a>"
        " or visit <a href='https://github.com/SSRVodka'>Github</a> :)")
        .arg(appName)
    );
    HIDE_POPUP;
}
