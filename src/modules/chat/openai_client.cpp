

#include <QtCore/QEventLoop>
#include <QtCore/QAtomicInteger>
#include <QtCore/QTimer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>

#include "modules/chat/openai_client.h"
#include "utils/logger.h"

using namespace Chat;

#define CLIENT_TYPE "OpenAI Client"


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
    
    QNetworkRequest request = createRequest();
    QNetworkReply* reply = m_manager->post(request, createRequestBody());
    
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
    
    QNetworkRequest request = createRequest();
    QNetworkReply* reply = m_manager->post(request, createRequestBody());
    
    QTimer* timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    
    // 注意 handleTimeout 和 handleResponse 的互斥条件
    connect(timeoutTimer, &QTimer::timeout, this, [this, reply, msgInHistory]() {
        if (this->m_currentFin.testAndSetRelaxed(false, true)) {
            // remove the message from the history if failed
            m_history.removeOne(msgInHistory);
            handleTimeout(reply);
        }
        // else: QNetworkReply::finished arrived first, so we do nothing here
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer]() {
        if (this->m_currentFin.testAndSetRelaxed(false, true)) {
            timeoutTimer->stop();
            handleResponse(reply);
            timeoutTimer->deleteLater();
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

QNetworkRequest Client::createRequest() const {
    QNetworkRequest request(m_serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty()) {
        QString token = QString("Bearer ") + m_apiKey;
        request.setRawHeader("Authorization", token.toUtf8());
    }
    return request;
}

QByteArray Client::createRequestBody() const {
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
    stdLogger.Debug(QJsonDocument(requestBody).toJson().toStdString());
    return QJsonDocument(requestBody).toJson();
}

Client::Message Client::addUserMessage(const QString& message) {
    Message msg {generateMsgId(), "user", message};
    m_history.append(msg);
    return msg;
}

Client::Message Client::addAssistantMessage(const QString& message) {
    Message msg {generateMsgId(), "assistant", message};
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

void Client::handleResponse(QNetworkReply* reply) {
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

void Client::handleTimeout(QNetworkReply* reply) {
    stdLogger.Warning(CLIENT_TYPE ": user message timeout (async)");
    if (m_pendingReplies.contains(reply)) {
        reply->abort();
        m_pendingReplies.remove(reply);
        reply->deleteLater();
        emit errorOccurred("Request timeout");
    } else {
        stdLogger.Exception("pending reply lost");
    }
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
    stdLogger.Debug(msg.c_str());
    m_serverUrl = params.server_url;

    stdLogger.Debug(CLIENT_TYPE ": api key is set to ***");
    m_apiKey = params.api_key;

    msg = CLIENT_TYPE ": chat model is set to " + params.model.toStdString();
    stdLogger.Debug(msg.c_str());
    m_model = params.model;

    msg = CLIENT_TYPE ": system prompt is set to: " + params.system_prompt.toStdString();
    stdLogger.Debug(msg.c_str());
    m_sysprompt = params.system_prompt;
}
void Client::setTimeout(int ms) {
    if (ms < 0) {
        // invalid parameter
        return;
    }
    std::string msg = CLIENT_TYPE ": client timeout (ms) is set to " + std::to_string(ms);
    stdLogger.Debug(msg.c_str());
    m_timeout = ms;
}
