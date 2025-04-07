/**
 * @file chatBox.h
 * @brief GUI rendering chat box widget.
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */

#pragma once

#include <QtWidgets/QDialog>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QSequentialAnimationGroup>

#include "gui/messagebubble.h"
#include "gui/popup.h"

#include "modules/audio/audio_handler.h"

#include "modules/chat/openai_client.h"

#include "ui_chatBox.h"

class mainWindow;

class ChatBox: public QDialog, public Ui::ChatBox {
    Q_OBJECT
public:
    ChatBox(mainWindow *parent /* not nullptr */);
    ~ChatBox();

private:
    void initAnimation();
    void loadStyleSheet();

    void loadHistory();

    MessageBubble *addMessageBubble(const QString &text, bool isUser);

    // void playSound(const QString &text);
    // void playSoundFromFile(const QString &audio_file);
    
    void toNormUIState();
    void toRecUIState();
    void toSTTRecogUIState();
    void toRcvUIState();

    Popup* popup;
    QPropertyAnimation* animeIn;
    QPropertyAnimation* animeOut;

    QSequentialAnimationGroup* seqGroup;

    mainWindow *main;
    // used to render error svg
    MessageBubble *last_pending_msg;

private slots:
    // only used to update the states (bool variables & message bubbles) of ChatBox.
    // logging & other staff is finished in mainWindow
    void recv_stt_reply(bool valid, QString transcribed_text);
    void recv_chat_reply(QString text);
    void recv_chat_error(QString msg);

    void on_sendBtn_clicked();
    void on_configBtn_clicked();
    void on_recordBtn_clicked();
};
