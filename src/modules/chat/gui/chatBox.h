#pragma once

#include <QtWidgets/QDialog>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QSequentialAnimationGroup>

#include "modules/chat/gui/popup.h"
#include "ui_chatBox.h"


class ChatBox: public QDialog, public Ui::ChatBox {
    Q_OBJECT
public:
    ChatBox(QWidget *parent = nullptr);
    ~ChatBox();

private:
    void initAnimation();
    void loadStyleSheet();

    Popup* popup;
    QPropertyAnimation* animeIn;
    QPropertyAnimation* animeOut;

    QSequentialAnimationGroup* seqGroup;


private slots:
    void on_sendBtn_clicked();
    void on_configBtn_clicked();
};
