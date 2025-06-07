/**
 * @file messagebubble.h
 * @brief GUI rendering message bubble widget.
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */

#pragma once

#include <QtCore/QDateTime>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

class MessageBubble : public QWidget
{
    Q_OBJECT
public:
    explicit MessageBubble(const QString &text, const QDateTime &time, const QString &role, QWidget *parent = nullptr);

    void setText(QString text);
    void showErrorIndicator();
signals:
    void textChanged();
private:

    QLabel *errorIcon = nullptr;  // 错误图标
    QLabel *textRef;
    QHBoxLayout *bubbleLayout;
    QWidget *_contentWidget;
    QHBoxLayout *_contentLayout;
    bool error;
};
