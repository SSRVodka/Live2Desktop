/**
 * @file mainWindow.h
 * @brief Main window for the application.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <QtCore/QJsonArray>
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

namespace mcp {
class client;
class tool;
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
    void recv_chat_async_reply(QString text);
    void recv_chat_stream_ready(QString chunk);
    void recv_chat_stream_fin();
    void recv_chat_error(QString msg);
    void recv_tool_calls(QJsonArray tool_calls);

    void toggle_keyboard_record();
private:
    void writeSettings();
    void loadSettings();
    void loadStyleSheet();
    
    void loadModels();
    void setupTray();

    void initMCPServers();
    void initClients();
    void initGlobalHotKey();

    // 按照模型指令调用指定工具（可能有多个），这会更改 Chat Client 的历史记录
    // 调用错误时（例如指定的 tool 不存在）会直接向历史记录中追加
    void callingTools(const QJsonArray &tool_calls);

    QJsonArray mcpTools2OAIFormatQJsonArray(std::vector<mcp::tool> mcp_tool);

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

    // tool calling & MCP utilities
    mcp::client *mcp_client;

    // global hotkey
    GlobalHotKeyHandler *hotkey_handler;
    bool is_keyboard_recording;

    QString current_stream_reply_buf;
};