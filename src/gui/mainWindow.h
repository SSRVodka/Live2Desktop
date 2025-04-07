/**
 * @file mainWindow.h
 * @brief Main window for the application.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSystemTrayIcon>

#include <sv.cpp/sense-voice/include/asr_handler.hpp>
#include "utils/consts.h"

#include "ui_mainWindow.h"

class AudioHandler;
class GlobalHotKeyHandler;

namespace Chat {
class Client;
};

class MCPConfig;
class ModuleConfigManager;

class mainWindow : public QMainWindow, public Ui::mainWindow {
    Q_OBJECT
    friend class ChatBox;
public:

    mainWindow(QApplication* mapp = nullptr);
    ~mainWindow();

protected:
    void closeEvent(QCloseEvent * e) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void exitApp();
    void switchClicked(QAction* curAct);
    void geoEditMode(bool enter);
    void config();
    void chatBegin();

    void aboutAuthor();

    void recv_stt_reply(bool valid, QString transcribed_text);
    void recv_tts_reply(bool success, QString msg);
    void recv_chat_reply(QString text);
    void recv_chat_error(QString msg);

    void toggle_keyboard_record();
private:
    void writeSettings();
    void loadSettings();
    void loadStyleSheet();
    
    void loadModels();
    void setupTray();

    void initClients();
    void initGlobalHotKey();

    QSystemTrayIcon* systemTray;
    QMenu *mainMenu;

    QMenu *switchMenu;
    QList<QAction*> modelList;
    QActionGroup *switchActionGroup;
    QAction *exitAction;
    QAction *geoEditAction;
    QAction *settingsAction;
    QAction *chatAction;
    QAction *aboutAction;
    
    QApplication* app;

    // configurations
    MCPConfig *mcp_config;
    ModuleConfigManager *module_config_manager;
    ASRHandler::asr_params stt_params;
    TTS::tts_params_t tts_params;

    // audio utilities
    AudioHandler *audio_handler;
    bool is_recording;
    QString last_tts_pending_audio_file;

    // chat utilities
    Chat::Client *chat_client;
    bool is_receiving;

    // global hotkey
    GlobalHotKeyHandler *hotkey_handler;
    bool is_keyboard_recording;
};