#include <QtCore/QSettings>

#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtGui/QMouseEvent>

#include "config/mcp_config.h"
#include "config/module_config.h"

#include "gui/animeWidget.h"
#include "gui/configDialog.h"
#include "gui/mainWindow.h"
#include "drivers/modelManager.h"
#include "drivers/resourceLoader.h"

#include "gui/chatBox.h"

#include "modules/audio/audio_recorder.h"
#include "modules/hotkey/shortcut_handler.h"

#include "utils/logger.h"

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

    initClients();
    initGlobalHotKey();
}

mainWindow::~mainWindow() {
    writeSettings();
    stdLogger.Debug("Geometry configurations saved.");

    delete this->chat_client;
    delete this->audio_handler;

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
            QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon::Warning;
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
    if (this->is_keyboard_recording) {
        stdLogger.Exception("please stop your keyboard recording first");
        return;
    }

    PREPARE_FOR_POPUP;
    ChatBox chatBox(this);
    int res = chatBox.exec();
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

    module_config_manager->set_asr_params(this->stt_params);
    module_config_manager->set_tts_params(this->tts_params);
    if (module_config_manager->save()) {
        stdLogger.Info("Module configurations saved");
    } else {
        stdLogger.Exception("Failed to save module configurations");
    }

    // TODO: MCP Config
}

void mainWindow::loadSettings() {
    QSettings settings("SSRVodka Inc.", appName);
    restoreGeometry(settings.value("geometry").toByteArray());

    QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon::Warning;
    std::string loadMsg;
    // load from config
    module_config_manager = ModuleConfigManager::get_instance(MODULE_CONFIG_FILE_PATH);
    if (module_config_manager->load()) {
        this->stt_params = module_config_manager->get_asr_params();
        loadMsg = "load STT model (local inference): " + this->stt_params.model;
        stdLogger.Info(loadMsg);
        this->tts_params = module_config_manager->get_tts_params();
        loadMsg = "load TTS model service ep: " + this->tts_params.server_url;
        stdLogger.Info(loadMsg);
    } else {
        stdLogger.Exception("Failed to load module configurations");
        this->systemTray->showMessage(
            appName,
            tr("Failed to load module configurations"),
            msgIcon, 5000
        );
    }

    // MCP config
    mcp_config = MCPConfig::getInstance();
    if (!mcp_config->load({MCP_CONFIG_FILE_PATH})) {
        stdLogger.Exception("Failed to load MCP configurations");
        this->systemTray->showMessage(
            appName,
            tr("Failed to load MCP configurations"),
            msgIcon, 5000
        );
    }
}

void mainWindow::loadStyleSheet() {
    QFile file(":/style.qss");
	if(file.open(QFile::ReadOnly)){
		QString styleSheet = QLatin1String(file.readAll());
    	this->setStyleSheet(styleSheet);
    	file.close();
	} else {
        QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon::Warning;
        this->systemTray->showMessage(
            appName,
            tr("Failed to load page style sheet."),
            msgIcon, 5000
        );
    }
}

void mainWindow::initClients() {
    this->audio_handler = new AudioHandler;
    this->chat_client = new Chat::Client;
    this->is_receiving = false;
    this->is_recording = false;

    // naive implementation: read JSON file directly

    // preparing for client parameters
    this->audio_handler->set_stt_params(this->stt_params);
    this->audio_handler->set_tts_params(this->tts_params);
    Chat::Client::chat_params_t cp;
    std::string chat_url = this->mcp_config->llm.base_url.value_or("") + "/chat/completions";
    cp.server_url = chat_url.data();
    cp.api_key = this->mcp_config->llm.api_key.value_or("").data();
    cp.model = this->mcp_config->llm.model.data();
    cp.system_prompt = this->mcp_config->system_prompt.data();
    this->chat_client->setChatParams(cp);
    this->chat_client->setTimeout(120000);
    this->chat_client->setUseStream(this->mcp_config->llm.stream);

    connect(this->audio_handler, SIGNAL(stt_reply(bool,QString)),
        this, SLOT(recv_stt_reply(bool, QString)));
    connect(this->audio_handler, SIGNAL(tts_reply(bool,QString)),
        this, SLOT(recv_tts_reply(bool, QString)));
    connect(this->chat_client, SIGNAL(asyncResponseReceived(const QString&)),
        this, SLOT(recv_chat_async_reply(QString)));
    connect(this->chat_client, SIGNAL(streamResponseReceived(const QString&)),
        this, SLOT(recv_chat_stream_ready(QString)));
    connect(this->chat_client, SIGNAL(streamFinished()),
        this, SLOT(recv_chat_stream_fin()));
    connect(this->chat_client, SIGNAL(errorOccurred(const QString&)),
        this, SLOT(recv_chat_error(QString)));
}

void mainWindow::initGlobalHotKey() {
    this->hotkey_handler = new GlobalHotKeyHandler;
    this->is_keyboard_recording = false;

    connect(this->hotkey_handler, SIGNAL(hotKeyActivate()),
        this, SLOT(toggle_keyboard_record()));
}
void mainWindow::toggle_keyboard_record() {
    if (this->is_recording) return; // conflict
    QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon::Information;

    if (this->is_keyboard_recording) {
        this->is_keyboard_recording = false;
        QString fn = this->audio_handler->get_recorder_unsafe_ptr()->record_stop();
        if (fn.isEmpty()) {
            msgIcon = QSystemTrayIcon::MessageIcon::Warning;
            stdLogger.Exception("failed to record: recorder error");
            this->systemTray->showMessage(
                appName,
                tr("failed to record: recorder error"),
                msgIcon, 5000
            );
            return;
        }
        this->audio_handler->stt_request(fn);
    } else {
        this->is_keyboard_recording = true;
        this->audio_handler->get_recorder_unsafe_ptr()->record();
        this->systemTray->showMessage(
            appName,
            tr("start audio recording"),
            msgIcon, 5000
        );
    }
}
void mainWindow::recv_stt_reply(bool valid, QString transcribed_text) {
    // whatever it comes from (keyboard or chatbox), just send it!
    if ((valid && transcribed_text.isEmpty()) || !valid) {
        stdLogger.Warning("empty speech text from STT client");
        return;
    }
    this->is_receiving = true;
    this->chat_client->sendMessageAsync(transcribed_text);
}
void mainWindow::recv_tts_reply(bool success, QString msg) {
    if (success) {
        // play sound from file
        stdLogger.Info("playing generated audio");
        this->audio_handler->get_recorder_unsafe_ptr()->play(this->last_tts_pending_audio_file);
        // start model lip-sync
        this->animeWidget->startLipSync(this->last_tts_pending_audio_file.toStdString());
    } else {
        stdLogger.Exception("failed to play audio '"
            + this->last_tts_pending_audio_file.toStdString()
            + "' due to: " + msg.toStdString());
    }
}
void mainWindow::recv_chat_async_reply(QString text) {
    this->is_receiving = false;
    QString gen_audio_file = this->audio_handler->tts_request(text);
    // we don't care much about exception (only log), because sound is not important :)
    this->last_tts_pending_audio_file = gen_audio_file;
}
void mainWindow::recv_chat_stream_ready(QString chunk) {
    // TODO TTS stream
    this->current_stream_reply_buf += chunk;
}
void mainWindow::recv_chat_stream_fin() {
    // TODO TTS stream
    QString msg = this->current_stream_reply_buf;
    this->current_stream_reply_buf = "";
    this->recv_chat_async_reply(msg);
}
void mainWindow::recv_chat_error(QString msg) {
    this->is_receiving = false;
    stdLogger.Exception("failed to retrieve response message due to client error: "
        + msg.toStdString());
}

void mainWindow::aboutAuthor() {
    PREPARE_FOR_POPUP;
    QMessageBox::about(0, tr("About me & my program"),
    QString("<h2>%1</h2>"
        "<p>Copyright &copy; 2023-2025 SSRVodka Inc. "
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
