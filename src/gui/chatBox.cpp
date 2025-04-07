
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSystemTrayIcon>

#include "gui/chatBox.h"
#include "gui/mainWindow.h"

#include "modules/audio/audio_recorder.h"

#include "utils/consts.h"
#include "utils/logger.h"


ChatBox::ChatBox(mainWindow *parent)
    :QDialog(parent),
    main(parent),
    last_pending_msg(nullptr) {
    
    this->setupUi(this);
    QWidget *scrollWidget = new QWidget;
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setAlignment(Qt::AlignTop);
    scrollLayout->setContentsMargins(5, 5, 5, 5);
    this->scrollArea->setWidget(scrollWidget);
    this->scrollArea->setWidgetResizable(true);

    // restore UI state
    this->toNormUIState();

    // connect to clients in mainWindow to get message update
    connect(this->main->audio_handler, SIGNAL(stt_reply(bool,QString)),
        this, SLOT(recv_stt_reply(bool, QString)));
    connect(this->main->chat_client, SIGNAL(asyncResponseReceived(const QString&)),
        this, SLOT(recv_chat_reply(QString)));
    connect(this->main->chat_client, SIGNAL(errorOccurred(const QString&)),
        this, SLOT(recv_chat_error(QString)));

    popup = new Popup(
        QString("✨ Welcome to %1! ✨")
        .arg(appName),
        this
    );
    popup->show();
    initAnimation();
    seqGroup->start();

    loadHistory();
}

ChatBox::~ChatBox() {}

void ChatBox::toNormUIState() {
    this->main->is_recording = false;
    this->main->is_receiving = false;
    this->inputEdit->setEnabled(true);
    this->sendBtn->setEnabled(true);
    this->recordBtn->setEnabled(true);
    this->recordBtn->setText(tr("record"));
    this->configBtn->setEnabled(true);
}
void ChatBox::toRecUIState() {
    this->main->is_receiving = false;
    this->main->is_recording = true;
    this->inputEdit->setEnabled(false);
    this->recordBtn->setEnabled(true);
    this->recordBtn->setText(tr("stop"));
    this->sendBtn->setEnabled(false);
    this->configBtn->setEnabled(false);
}
void ChatBox::toSTTRecogUIState() {
    this->main->is_receiving = false;
    this->main->is_recording = true;
    this->inputEdit->setEnabled(false);
    this->recordBtn->setEnabled(false);
    this->recordBtn->setText(tr("wait"));
    this->sendBtn->setEnabled(false);
    this->configBtn->setEnabled(false);
}
void ChatBox::toRcvUIState() {
    this->main->is_receiving = true;
    this->main->is_recording = false;
    this->inputEdit->setEnabled(false);
    this->recordBtn->setEnabled(false);
    this->recordBtn->setText(tr("record"));
    this->sendBtn->setEnabled(false);
    this->configBtn->setEnabled(false);
}

MessageBubble *ChatBox::addMessageBubble(const QString &text, bool isUser) {
    QWidget *scrollWidget = this->scrollArea->widget();
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(scrollWidget->layout());
    
    MessageBubble *bubble = new MessageBubble(text, QDateTime::currentDateTime(), isUser, this);
    layout->addWidget(bubble);

    // 自动滚动到底部
    QTimer::singleShot(500, [this](){
        this->scrollArea->verticalScrollBar()->setValue(
            this->scrollArea->verticalScrollBar()->maximum()
        );
    });

    // 限制历史消息数量（超过100条时删除最早的）
    if(layout->count() > 100){
        QLayoutItem* item = layout->takeAt(0);
        delete item->widget();
        delete item;
    }

    return bubble;
}

void ChatBox::loadHistory() {
    QVector<Chat::Client::Message> history = this->main->chat_client->getHistory();
    foreach (const Chat::Client::Message &message, history) {
        this->addMessageBubble(message.content, message.role == "user");
    }
}

void ChatBox::on_sendBtn_clicked() {
    if (this->main->is_receiving) return;

    QString text = this->inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    // update UI
    this->inputEdit->clear();
    this->toRcvUIState();
    this->last_pending_msg = this->addMessageBubble(text, true);

    // send to server
    this->main->chat_client->sendMessageAsync(text);
}
void ChatBox::on_configBtn_clicked() {
    QMessageBox::information(
        this, QString("Dev Info"), QString("under development")
    );
}
void ChatBox::on_recordBtn_clicked() {
    if (this->main->is_keyboard_recording) return;  // conflict
    if (this->main->is_recording) {
        // start STT process
        this->toSTTRecogUIState();

        QString fn = this->main->audio_handler->get_recorder_unsafe_ptr()->record_stop();
        if (fn.isEmpty()) {
            stdLogger.Exception("failed to record: recorder error");
            this->toNormUIState();
            return;
        }
        this->main->audio_handler->stt_request(fn);
    } else {
        // start recording
        this->toRecUIState();
        this->main->audio_handler->get_recorder_unsafe_ptr()->record();
    }
}
void ChatBox::recv_stt_reply(bool valid, QString transcribed_text) {
    // restore UI state
    this->toNormUIState();
    if ((valid && transcribed_text.isEmpty()) || !valid) {
        return;
    }
    this->inputEdit->setText(transcribed_text);
    // 自动发送识别信息
    this->on_sendBtn_clicked();
}
void ChatBox::recv_chat_reply(QString text) {
    this->addMessageBubble(text, false);
    this->last_pending_msg = nullptr;
    this->toNormUIState();
}
void ChatBox::recv_chat_error(QString msg) {
    this->last_pending_msg->showErrorIndicator();
    this->last_pending_msg = nullptr;
    this->toNormUIState();
}

void ChatBox::initAnimation() {
    animeIn = new QPropertyAnimation(popup, "geometry");
    animeIn->setStartValue(QRect(0, 0, width(), 0));
    animeIn->setEndValue(QRect(0, 0, width(), popupHeight));
    animeIn->setDuration(scrollInDuration);

    animeOut = new QPropertyAnimation(popup, "geometry");
    animeOut->setStartValue(QRect(0, 0, width(), popupHeight));
    animeOut->setEndValue(QRect(0, 0, width(), 0));
    animeOut->setDuration(scrollOutDuration);

    seqGroup = new QSequentialAnimationGroup(this);
    seqGroup->addAnimation(animeIn);
    seqGroup->addPause(defaultDuration);
    seqGroup->addAnimation(animeOut);
}

void ChatBox::loadStyleSheet() {
    QFile file(":/style.qss");
	if (file.open(QFile::ReadOnly)) {
		QString styleSheet = QLatin1String(file.readAll());
    	this->setStyleSheet(styleSheet);
    	file.close();
	} else {
        stdLogger.Warning("Failed to load style sheet for ChatBox.");
    }
}
