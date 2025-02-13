#pragma once

#include "ui_popup.h"

class Popup : public QWidget, public Ui::popup
{
public:
    explicit Popup(const QString& showStr, QWidget* parent = nullptr);
    ~Popup();

    void setText(const QString& txt) { text->setText(txt); }

private:
    QString showStr;
};
