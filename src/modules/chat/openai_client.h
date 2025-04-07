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
    QVector<Message> getHistory();
    void clearHistory();

signals:
    void asyncResponseReceived(const QString& response);
    void errorOccurred(const QString& error);

private:
    inline QNetworkRequest createRequest() const;
    inline QByteArray createRequestBody() const;

    inline Message addUserMessage(const QString& message);
    inline Message addAssistantMessage(const QString& message);

    std::pair<QString, bool> processReply(QNetworkReply* reply);

    void handleResponse(QNetworkReply* reply);

    void handleTimeout(QNetworkReply* reply);

    void abortAllRequests();

    quint32 generateMsgId();

    QNetworkAccessManager* m_manager;
    QString m_serverUrl;
    QString m_model;
    QString m_apiKey;
    QString m_sysprompt;
    QVector<Message> m_history;
    QSet<QNetworkReply*> m_pendingReplies;

    QAtomicInteger<quint32> m_nextMsgId;
    // (仅对异步操作) 当前请求是否已经确认结果（不再 pending），可以是超时或者回应
    QAtomicInteger<qint8> m_currentFin;
    int m_timeout;
};

};
