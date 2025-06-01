/**
 * @file openai_client.h
 * @brief HTTP client class supports OpenAI API.
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */

#pragma once

#include <QtCore/QMutex>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QTimer>

namespace Chat {

/**
 * @brief HTTP client class supports OpenAI API
 * @warning This class is not thread-safe for now
 * @todo TODO: 1. ensure thread-safety; 2. persist message data
 */
class Client : public QObject {
    Q_OBJECT
public:
    struct Message {
        quint32 id;
        QString role;
        QString content;
        bool operator==(const Message &other) {
            return this->id == other.id;
        }
    };

    struct chat_params_t {
        QString server_url;
        QString api_key;
        QString model;
        QString system_prompt;
    };

    explicit Client(int timeoutMs = 30000, 
                    QObject* parent = nullptr);
    ~Client();

    std::pair<QString, bool> sendMessageSync(const QString& message);
    void sendMessageAsync(const QString& message);

    void setChatParams(const chat_params_t &params);
    void setTimeout(int ms);
    void setUseStream(bool stream);
    QVector<Message> getHistory();
    void clearHistory();

signals:
    void asyncResponseReceived(const QString& response);
    void streamResponseReceived(const QString& chunk);
    void streamFinished();
    void errorOccurred(const QString& error);

private:
    inline QNetworkRequest createRequest(bool stream) const;
    inline QByteArray createRequestBody(bool stream) const;

    inline Message addUserMessage(const QString& message);
    inline Message addAssistantMessage(const QString& message);

    // 处理一般回复（同步 / 一般异步）
    std::pair<QString, bool> processReply(QNetworkReply* reply);
    // 处理流式回复
    void processStreamBuffer(QNetworkReply* reply);
    // 处理单个SSE事件的工具函数
    void processStreamEvent(QNetworkReply* reply, const QByteArray& eventData);

    void handleAsyncResponse(QNetworkReply* reply);
    void handleAsyncTimeout(QNetworkReply* reply);
    void handleStreamFinished(QNetworkReply *reply);
    void handleStreamTimeout(QNetworkReply *reply);

    void abortAllRequests();

    quint32 generateMsgId();

    QNetworkAccessManager* m_manager;
    QString m_serverUrl;
    QString m_model;
    QString m_apiKey;
    QString m_sysprompt;
    QVector<Message> m_history;
    QSet<QNetworkReply*> m_pendingReplies;

    // 流式传输上下文
    struct StreamContext {
        QByteArray buffer;          // 原始数据缓冲区
        QString accumulatedResponse; // 累积的完整响应
        QTimer* timeoutTimer;        // 关联的超时定时器
        Message userMessage;         // 关联的用户消息
    };
    
    QHash<QNetworkReply*, StreamContext> m_streamContexts;

    QAtomicInteger<quint32> m_nextMsgId;
    // (仅对异步操作) 当前请求是否已经确认结果（不再 pending），可以是超时或者回应
    QAtomicInteger<qint8> m_currentFin;
    int m_timeout;
    bool m_stream;
};

};
