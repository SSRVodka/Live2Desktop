
#include <QtCore/QFile>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSystemTrayIcon>

#include "modules/chat/gui/chatBox.h"
#include "utils/consts.h"
#include "utils/logger.h"


ChatBox::ChatBox(QWidget *parent)
    :QDialog(parent) {
    this->setupUi(this);

    popup = new Popup(
        QString("✨ Welcome to %1! ✨")
        .arg(appName),
        this
    );
    popup->show();
    initAnimation();
    seqGroup->start();
}

ChatBox::~ChatBox() {}


void ChatBox::on_sendBtn_clicked() {
    QMessageBox::information(
        this, QString("Dev Info"), QString("under development")
    );
}
void ChatBox::on_configBtn_clicked() {
    QMessageBox::information(
        this, QString("Dev Info"), QString("under development")
    );
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
