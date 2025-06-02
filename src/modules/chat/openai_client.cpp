

#include <QtCore/QEventLoop>
#include <QtCore/QAtomicInteger>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>

#include "modules/chat/openai_client.h"
#include "utils/logger.h"

using namespace Chat;

#define CLIENT_TYPE "OpenAI Client"


// 注意：流式传输和非流式传输的处理方法不同。

// 非流式：

// - 直接发送消息；
// - 未回复的消息由 m_pendingReplies 管理；
// - 使用 QNetworkReply::finished 处理回复；
// - 使用 Client::handleAsyncTimeout 处理超时；
// - 使用 Client::handleAsyncResponse 更新等待状态、解析回复信息、发出完成信号；

// 流式：

// - 和非流式一样正常发送消息，但是加上特殊报头；
// - QNetworkReply::readyRead 处理流式数据到达；
// - 



Client::Client(
    int timeoutMs, 
    QObject* parent)
: QObject(parent)
, m_manager(new QNetworkAccessManager(this))
, m_serverUrl("http://localhost:80")
, m_apiKey("")
, m_model("gpt-3.5-turbo")
, m_sysprompt("")
, m_timeout(timeoutMs) {}

Client::~Client()  {
    abortAllRequests();

    delete this->m_manager;
}

quint32 Client::generateMsgId() {
    return m_nextMsgId.fetchAndAddRelaxed(1);
}

std::pair<QString, bool> Client::sendMessageSync(const QString& message) {
    stdLogger.Info(CLIENT_TYPE ": user send message (sync) to server");
    Message msgInHistory = addUserMessage(message);
    
    QNetworkRequest request = createRequest(false);
    QNetworkReply* reply = m_manager->post(request, createRequestBody(false));
    
    QEventLoop eventLoop;
    QTimer timeoutTimer;
    
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, [&eventLoop]() {
        eventLoop.exit(1);
    });
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);

    timeoutTimer.start(m_timeout);
    int result = eventLoop.exec();

    QString response;
    bool success = false;

    if (result == 0) {
        std::tie(response, success) = processReply(reply);
    } else { // 超时
        stdLogger.Warning(CLIENT_TYPE ": user message timeout (sync)");
        reply->abort();
        response = "Request timeout";
        // remove the message from the history if failed
        m_history.removeOne(msgInHistory);
    }

    reply->deleteLater();
    return {response, success};
}

void Client::sendMessageAsync(const QString& message) {
    stdLogger.Info(CLIENT_TYPE ": user send message (async) to server");
    Message msgInHistory = addUserMessage(message);

    this->m_currentFin.storeRelaxed(0);

    bool stream = m_stream;
    
    QNetworkRequest request = createRequest(stream);
    QNetworkReply* reply = m_manager->post(request, createRequestBody(stream));
    
    QTimer* timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);

    if (stream) {
        m_streamContexts[reply] = {
            QByteArray(),       // 空缓冲区
            QString(),          // 空累积响应
            timeoutTimer,       // 关联定时器
            msgInHistory        // 用户消息
        };

        connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
            auto& context = m_streamContexts[reply];
            
            // 收到数据时重置超时计时器
            if (context.timeoutTimer->isActive()) {
                context.timeoutTimer->start(m_timeout);
            }
            
            // 累积数据到缓冲区
            context.buffer += reply->readAll();
            
            // 解析完整事件 (以\n\n分隔)
            processStreamBuffer(reply);
        });
    }
    
    // 注意 handleAsyncTimeout 和 handleAsyncResponse 的互斥条件
    connect(timeoutTimer, &QTimer::timeout, this, [this, reply, msgInHistory, stream]() {
        if (this->m_currentFin.testAndSetRelaxed(false, true)) {
            if (m_stream) {
                handleStreamTimeout(reply);
            } else {
                // remove the message from the history if failed
                m_history.removeOne(msgInHistory);
                handleAsyncTimeout(reply);
            }
        }
        // else: QNetworkReply::finished arrived first, so we do nothing here
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer, stream]() {
        if (this->m_currentFin.testAndSetRelaxed(false, true)) {
            if (stream) {
                // 由于 StreamContext 的封装，这里计时器停止的工作已经下沉到函数中进行。
                handleStreamFinished(reply);
            } else {
                timeoutTimer->stop();
                handleAsyncResponse(reply);
                timeoutTimer->deleteLater();
            }
        }
    });

    timeoutTimer->start(m_timeout);
    m_pendingReplies.insert(reply);
}

QVector<Client::Message> Client::getHistory() {
    return this->m_history;
}

void Client::clearHistory() {
    stdLogger.Debug(CLIENT_TYPE ": message history is clear");
    m_history.clear();
}


QString Client::removeTags(const char *tagName, const QString &text) {
    QString result = text;
    QString openTag = QString::asprintf("<%s>", tagName);
    QString closeTag = QString::asprintf("</%s>", tagName);

    int startIndex = 0;
    while ((startIndex = result.indexOf(openTag, startIndex)) != -1) {
        int endIndex = result.indexOf(closeTag, startIndex);
        if (endIndex != -1) {
            // 删除包括标签在内的整个内容
            result.remove(startIndex, endIndex - startIndex + closeTag.length());
        } else {
            result.remove(startIndex, result.length() - startIndex);
            break;
        }
    }
    
    return result.trimmed();
}
QString Client::removeCodeBlocks(const QString &text) {
    QString result = text;
    QString startCodeTag = "```";
    QString endCodeTag = startCodeTag;
    
    int startCodeIndex = 0;
    while ((startCodeIndex = result.indexOf(startCodeTag, startCodeIndex)) != -1) {
        int endCodeIndex = result.indexOf(endCodeTag, startCodeIndex + startCodeTag.length());
        if (endCodeIndex != -1) {
            result.remove(startCodeIndex, endCodeIndex - startCodeIndex + endCodeTag.length());
        } else {
            result.remove(startCodeIndex, result.length() - startCodeIndex);
            break;
        }
    }
    return result.trimmed();
}

QNetworkRequest Client::createRequest(bool stream) const {
    QNetworkRequest request(m_serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (stream) {
        request.setRawHeader("Accept", "text/event-stream");
    }

    if (!m_apiKey.isEmpty()) {
        QString token = QString("Bearer ") + m_apiKey;
        request.setRawHeader("Authorization", token.toUtf8());
    }
    return request;
}

QByteArray Client::createRequestBody(bool stream) const {
    QJsonObject requestBody;
    QJsonArray messagesArray;
    QJsonObject messageObj;

    // system prompt
    if (!m_sysprompt.isEmpty()) {
        messageObj["role"] = "system";
        messageObj["content"] = m_sysprompt;
        messagesArray.append(messageObj);
    }
    // history
    for (const auto& msg : m_history) {
        messageObj["role"] = msg.role;
        messageObj["content"] = msg.content;
        messagesArray.append(messageObj);
    }
    
    std::string msg = CLIENT_TYPE ": create request body with history length = "
        + std::to_string(m_history.size());
    stdLogger.Debug(msg.c_str());
    requestBody["model"] = m_model;
    requestBody["messages"] = messagesArray;
    // [{
    //     "type": "web_search",
    //     "web_search": {
    //         "enable": True  # 启用网络搜索
    //     }
    // }]
    // QJsonArray toolArray;
    // QJsonObject toolObj, toolOptObj;
    // toolObj["type"] = "web_search";
    // toolOptObj["enable"] = true;
    // toolObj["web_search"] = toolOptObj;
    // toolArray.append(toolObj);
    // requestBody["tools"] = toolArray;

    if (stream) {
        requestBody["stream"] = true;
    }

    stdLogger.Debug(QJsonDocument(requestBody).toJson().toStdString());
    return QJsonDocument(requestBody).toJson();
}

Client::Message Client::addUserMessage(const QString& message) {
    Message msg {generateMsgId(), "user", message};
    m_history.append(msg);
    return msg;
}

Client::Message Client::addAssistantMessage(const QString& message) {
    // 注意：如果有 think 块则不放入历史记录中
    QString convMsg;
    if (m_thinking) {
        convMsg = Client::removeTags("think", message);
    } else {
        convMsg = message;
    }
    Message msg {generateMsgId(), "assistant", convMsg};
    m_history.append(msg);
    return msg;
}

std::pair<QString, bool> Client::processReply(QNetworkReply* reply) {
    std::string msg;
    if (reply->error() != QNetworkReply::NoError) {
        msg = CLIENT_TYPE ": server response error: "
            + reply->errorString().toStdString();
        stdLogger.Exception(msg.c_str());
        return {reply->errorString(), false};
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull()) {
        stdLogger.Exception(CLIENT_TYPE ": client side error: invalid JSON response (null)");
        return {"Invalid JSON response (null)", false};
    }

    const QJsonObject root = doc.object();
    const QString content = root["choices"].toArray().first()
                           .toObject()["message"].toObject()["content"].toString();
    
    if (content.isEmpty()) {
        stdLogger.Exception(CLIENT_TYPE ": client side error: empty or malformat response content\n"
            + doc.toJson().toStdString());
        return {"Empty response content", false};
    }
    stdLogger.Debug(doc.toJson().toStdString());
    stdLogger.Info(CLIENT_TYPE ": client received response");
    addAssistantMessage(content);
    return {content, true};
}

void Client::processStreamBuffer(QNetworkReply* reply) {
    auto& context = m_streamContexts[reply];
    
    // 解析所有完整事件 (以\n\n分隔)
    while (true) {
        int pos = context.buffer.indexOf("\n\n");
        if (pos == -1) break;  // 没有完整事件
        
        QByteArray eventData = context.buffer.left(pos);
        context.buffer = context.buffer.mid(pos + 2);
        
        processStreamEvent(reply, eventData);
    }
}

void Client::processStreamEvent(QNetworkReply* reply, const QByteArray& eventData) {
    stdLogger.Debug(CLIENT_TYPE ": stream event received. Ready to process");
    // 忽略注释行和空事件
    if (eventData.startsWith(":") || eventData.isEmpty()) 
        return;

    // 检查DONE事件
    if (eventData.startsWith("data: [DONE]")) {
        return;
    }

    // 提取有效数据部分
    if (eventData.startsWith("data: ")) {
        QByteArray jsonData = eventData.mid(6); // 跳过"data: "
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            stdLogger.Exception(CLIENT_TYPE ": SSE JSON parse error: " + parseError.errorString().toStdString());
            return;
        }
        
        QJsonObject obj = doc.object();
        QJsonArray choices = obj["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject choice = choices[0].toObject();
            QJsonObject delta = choice["delta"].toObject();
            
            if (delta.contains("content")) {
                QString chunk = delta["content"].toString();
                auto& context = m_streamContexts[reply];
                context.accumulatedResponse += chunk;
                emit streamResponseReceived(chunk);
            }
        }
    } else {
        stdLogger.Warning(CLIENT_TYPE ": invalid stream data: " + eventData.toStdString());
    }
}

void Client::handleAsyncResponse(QNetworkReply* reply) {
    m_pendingReplies.remove(reply);
    
    // auto [response, success] = processReply(reply);
    std::pair<QString, bool> rp = processReply(reply);
    QString response = rp.first;
    bool success = rp.second;
    reply->deleteLater();
    
    if (success) {
        emit asyncResponseReceived(response);
    } else {
        emit errorOccurred(response);
    }
}

void Client::handleAsyncTimeout(QNetworkReply* reply) {
    stdLogger.Warning(CLIENT_TYPE ": user message timeout (async)");
    if (m_pendingReplies.contains(reply)) {
        reply->abort();
        m_pendingReplies.remove(reply);
        reply->deleteLater();
        emit errorOccurred("Async request timeout");
    } else {
        stdLogger.Exception(CLIENT_TYPE ": pending reply lost");
    }
}

void Client::handleStreamFinished(QNetworkReply *reply) {
    if (!m_streamContexts.contains(reply)) {
        stdLogger.Warning(CLIENT_TYPE ": received a stream finish event but not recognized");
        return;
    }
    
    auto context = m_streamContexts.take(reply);
    context.timeoutTimer->stop();
    context.timeoutTimer->deleteLater();
    
    // 处理缓冲区中剩余数据
    if (!context.buffer.isEmpty()) {
        processStreamEvent(reply, context.buffer);
    }
    
    // 添加到历史记录
    if (!context.accumulatedResponse.isEmpty()) {
        addAssistantMessage(context.accumulatedResponse);
    }
    
    // 清理
    m_pendingReplies.remove(reply);
    reply->deleteLater();
    
    emit streamFinished();
}

void Client::handleStreamTimeout(QNetworkReply *reply) {
    if (!m_streamContexts.contains(reply)) {
        stdLogger.Warning(CLIENT_TYPE ": received a stream timeout event but not recognized");
        return;
    }
    
    auto context = m_streamContexts.take(reply);
    context.timeoutTimer->deleteLater();
    
    // 添加部分响应到历史记录
    if (!context.accumulatedResponse.isEmpty()) {
        addAssistantMessage(context.accumulatedResponse);
    } else {
        // 没有任何响应时回滚用户消息
        m_history.removeOne(context.userMessage);
    }
    
    // 清理
    m_pendingReplies.remove(reply);
    reply->abort();
    reply->deleteLater();
    
    emit errorOccurred("Stream Request timeout");
}

void Client::abortAllRequests() {
    stdLogger.Debug(CLIENT_TYPE ": client is aborting all the handling requests");
    for (QNetworkReply* reply : m_pendingReplies) {
        reply->abort();
        reply->deleteLater();
    }
    m_pendingReplies.clear();
}

void Client::setChatParams(const chat_params_t &params) {
    std::string msg = CLIENT_TYPE ": sever url is set to " + params.server_url.toStdString();
    stdLogger.Debug(msg);
    m_serverUrl = params.server_url;

    stdLogger.Debug(CLIENT_TYPE ": api key is set to ***");
    m_apiKey = params.api_key;

    msg = CLIENT_TYPE ": chat model is set to " + params.model.toStdString();
    stdLogger.Debug(msg);
    m_model = params.model;

    msg = CLIENT_TYPE ": system prompt is set to: " + params.system_prompt.toStdString();
    stdLogger.Debug(msg);
    m_sysprompt = params.system_prompt;

    msg = CLIENT_TYPE ": enable thinking is set to: " + std::to_string(params.enable_thinking);
    stdLogger.Debug(msg);
    m_thinking = params.enable_thinking;
    m_sysprompt += m_thinking ? " /think" : " /no_think";
}
void Client::setTimeout(int ms) {
    if (ms < 0) {
        // invalid parameter
        return;
    }
    std::string msg = CLIENT_TYPE ": client timeout (ms) is set to " + std::to_string(ms);
    stdLogger.Debug(msg);
    m_timeout = ms;
}
void Client::setUseStream(bool stream) {
    std::string msg = CLIENT_TYPE ": client use stream is set to " + std::to_string(stream);
    stdLogger.Debug(msg);
    m_stream = stream;
}
