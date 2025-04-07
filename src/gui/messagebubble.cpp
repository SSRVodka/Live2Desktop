
#include <QtWidgets/QStyle>
#include <QtWidgets/QVBoxLayout>

#include "gui/messagebubble.h"

// TODO: extract style string from code
MessageBubble::MessageBubble(const QString &text, const QDateTime &time, bool isUser, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);

    // 时间标签
    QLabel *timeLabel = new QLabel(time.toString("yyyy/MM/dd hh:mm:ss"));
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setStyleSheet("color: #666666; font-size: 10px;");
    mainLayout->addWidget(timeLabel);

    // 创建内容容器
    QWidget *contentWidget = new QWidget;
    QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    
    // 错误图标（初始隐藏）
    error = false;
    errorIcon = new QLabel;
    errorIcon->setFixedSize(16, 16);
    errorIcon->setVisible(false);
    contentLayout->addWidget(errorIcon);
    
    // 消息内容
    QLabel *msgLabel = new QLabel(text);
    msgLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    msgLabel->setCursor(Qt::IBeamCursor); 
    msgLabel->setWordWrap(true);
    msgLabel->setContentsMargins(10, 8, 10, 8);
    msgLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    msgLabel->setMaximumWidth(360);
    // // 在消息气泡中添加尺寸控制
    // msgLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    // contentWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    // 添加最小宽度保证
    // msgLabel->setMinimumWidth(50);

    // 设置样式
    QString style = QString(
        "background: %1;"
        "border-radius: 8px;"
        "color: black;"
    ).arg(isUser ? "#DCF8C6" : "#EAEAEA");
    msgLabel->setStyleSheet(style);

    contentLayout->addWidget(msgLabel);

    // 主气泡布局
    bubbleLayout = new QHBoxLayout;
    if (isUser) {
        // 用户消息：整体靠右
        bubbleLayout->addStretch();
        bubbleLayout->addWidget(contentWidget);  // 包含图标和消息的整体
    } else {
        // 系统消息：整体靠左
        bubbleLayout->addWidget(contentWidget);
        bubbleLayout->addStretch();
    }

    mainLayout->addLayout(bubbleLayout);
}

void MessageBubble::showErrorIndicator()
{
    if (!error) {
        error = true;
        errorIcon->setPixmap(style()->standardPixmap(QStyle::SP_MessageBoxWarning));
        errorIcon->setStyleSheet("color: #FF4444;");
    }
    errorIcon->setVisible(true);
}

