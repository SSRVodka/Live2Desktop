/**
 * @file openai_client.h
 * @brief HTTP client class supports OpenAI API.
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */

#pragma once

#include <QtCore/QJsonArray>
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
        QString role;           // 可能的角色："assistant", "user", "tool"
        QString content;
        QJsonArray tool_calls;  // 当 role="assistant" 时使用
        QString tool_call_id;   // 当 role="tool" 时使用

        bool operator==(const Message &other) {
            return this->id == other.id;
        }
    };

    struct chat_params_t {
        QString server_url;
        QString api_key;
        QString model;
        QString system_prompt;
        // only valid for reasoning model
        bool enable_thinking;
    };

    explicit Client(int timeoutMs = 30000, 
                    QObject* parent = nullptr);
    ~Client();

    std::pair<QString, bool> sendMessageSync(const QString& message);
    void sendMessageAsync(const QString& message);

    // 该方法不会更改历史记录，只会将当前历史记录发给模型继续生成。
    // 注意：外部调用方只能在工具调用返回结果补充到历史记录后，转移控制流时调用
    void continueConversation();
    Message addToolMessage(const QString& tool_call_id, const QString& content);


    void setChatParams(const chat_params_t &params);
    void setTimeout(int ms);
    void setUseStream(bool stream);
    void setTools(const QJsonArray& tools);
    QVector<Message> getHistory();
    void clearHistory();

    // 处理回复字符串的工具函数
    static QString removeTags(const char *tagName, const QString &text);
    static QString removeCodeBlocks(const QString &text);

signals:
    void asyncResponseReceived(const QString& response);
    void streamResponseReceived(const QString& chunk);
    void streamFinished();
    void toolCallsReceived(const QJsonArray& tool_calls);
    void errorOccurred(const QString& error);

private:
    inline QNetworkRequest createRequest(bool stream) const;
    inline QByteArray createRequestBody(bool stream) const;

    inline Message addUserMessage(const QString& message);
    // @param tool_calls nullptr represents no tool calls
    inline Message createAssistantMessage(const QString& message, QJsonArray *tool_calls = nullptr);
    inline void addAssistantMessage(Message &msg);

    // 处理一般回复（同步 / 一般异步）
    // 注：不会将 Message 加入 history
    std::pair<Message, bool> processReply(QNetworkReply* reply);
    // 处理流式回复
    void processStreamBuffer(QNetworkReply* reply);
    // 处理单个SSE事件的工具函数
    void processStreamEvent(QNetworkReply* reply, const QByteArray& eventData);

    void handleAsyncResponse(QNetworkReply* reply);
    void handleAsyncTimeout(QNetworkReply* reply, Message relatedMsg);
    void handleStreamFinished(QNetworkReply *reply);
    void handleStreamTimeout(QNetworkReply *reply);

    // 流式传输时，工具调用也可能是 partial 的（尤其是 arguments 比较长的时候），需要工具函数拼接
    void mergeToolCallsStreamDeltaTo(QJsonArray partialToolCalls, QJsonArray *dst);
    // 作用：为某些不符合 OpenAI Format 的接口提供兼容性
    // 目前仅支持工具调用类型为 function
    QJsonArray formatToolCalls2OAIFormat(QJsonArray tool_calls);

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
        Message lastMessage;         // 关联的用户消息或者工具消息（上一条）
        bool hasToolCalls;          // 当前流式事件是否为工具调用请求，而非一般的流式信息
        QJsonArray tool_calls;      // 当前流式事件中的工具调用请求
    };
    
    QHash<QNetworkReply*, StreamContext> m_streamContexts;

    QAtomicInteger<quint32> m_nextMsgId;
    // (仅对异步操作) 当前请求是否已经确认结果（不再 pending），可以是超时或者回应
    QAtomicInteger<qint8> m_currentFin;
    
    QJsonArray m_tools;
    int m_timeout;
    bool m_thinking;
    bool m_stream;
};

};
