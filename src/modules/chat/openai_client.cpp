

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
// - 使用 QNetworkReply::finished 处理回复、QNetworkReply::timeout 处理超时；
// - 使用 Client::handleAsyncTimeout 处理超时；
// - 使用 Client::handleAsyncResponse 更新等待状态、解析回复信息、发出完成信号；

// 流式：

// - 和非流式一样正常发送消息，但是加上特殊报头；
// - QNetworkReply::readyRead 处理流式数据到达；
// - 超时和完成的信号与非流式相同（finished, timeout）；
// - 使用 Client::handleStreamTimeout / Client::handleStreamFinished 则对应非流式的相应功能



Client::Client(
    int timeoutMs, 
    QObject* parent)
: QObject(parent)
, m_manager(new QNetworkAccessManager(this))
, m_serverUrl("http://localhost:80")
, m_apiKey("")
, m_model("gpt-3.5-turbo")
, m_sysprompt("")
, m_currentFin(1)
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

    Message replyMsg;
    bool success = false;

    if (result == 0) {
        std::tie(replyMsg, success) = processReply(reply);
        if (replyMsg.tool_calls.size() > 0) {
            return {"Tool calls not supported in sync mode", false};
        }
    } else { // 超时
        stdLogger.Warning(CLIENT_TYPE ": user message timeout (sync)");
        reply->abort();
        replyMsg.content = "Request timeout";
    }

    if (success) {
        addAssistantMessage(replyMsg);
    }

    reply->deleteLater();
    return {replyMsg.content, success};
}

void Client::sendMessageAsync(const QString& message) {
    stdLogger.Info(CLIENT_TYPE ": user send message (async) to server");
    addUserMessage(message);

    continueConversation();
}

void Client::continueConversation() {

    Message lastMsg = m_history.last();

    if (!m_currentFin.testAndSetRelaxed(true, false)) {
        stdLogger.Exception("last conversation is not ended");
        return;
    }
    if (!m_pendingReplies.isEmpty()) {
        stdLogger.Exception("Data inconsistency: pending replies still exist");
        return;
    }

    bool stream = m_stream;
    
    QNetworkRequest request = createRequest(stream);
    
    QTimer* timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    QNetworkReply* reply = m_manager->post(request, createRequestBody(stream));
    m_pendingReplies.insert(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer, stream]() {
        if (this->m_currentFin.testAndSetRelaxed(false, true)) {
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
            if (stream) {
                handleStreamFinished(reply);
            } else {
                handleAsyncResponse(reply);
            }
        }
    });
    if (stream) {
        m_streamContexts[reply] = {
            QByteArray(),       // 空缓冲区
            QString(),          // 空累积响应
            timeoutTimer,       // 关联定时器
            lastMsg,            // 上一条消息（关联消息，可以是用户消息也可以是工具消息）
            false               // 是否有工具调用
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
    connect(timeoutTimer, &QTimer::timeout, this, [this, reply, lastMsg, stream]() {
        if (this->m_currentFin.testAndSetRelaxed(false, true)) {
            if (stream) {
                // 因为流式输出即便超时也可能有部分输出，用户可以通过 “继续” 提示模型继续接着输出，
                // 所以真正判断是否需要回滚用户消息的逻辑下沉到此函数中
                handleStreamTimeout(reply);
            } else {
                // 这里回滚用户消息的逻辑也下沉到此函数中，因为需要判断 lastMsg 是否是用户消息
                handleAsyncTimeout(reply, lastMsg);
            }
        }
        // else: QNetworkReply::finished arrived first, so we do nothing here
    });
    timeoutTimer->start(m_timeout);
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

    {
        QJsonObject messageObj;
        // system prompt
        if (!m_sysprompt.isEmpty()) {
            messageObj["role"] = "system";
            messageObj["content"] = m_sysprompt;
            messagesArray.append(messageObj);
        }
    }
    // history
    for (const auto& msg : m_history) {
        QJsonObject messageObj;
        messageObj["role"] = msg.role;
        // support tool calling
        if (msg.role == "tool") {
            messageObj["content"] = msg.content;
            messageObj["tool_call_id"] = msg.tool_call_id;
        } else if (msg.role == "assistant" && !msg.tool_calls.empty()) {
            if (!msg.content.isEmpty()) {
                messageObj["content"] = msg.content;
            }
            messageObj["tool_calls"] = msg.tool_calls;
        } else {
            messageObj["content"] = msg.content;
        }
        messagesArray.append(messageObj);
    }
    
    std::string msg = CLIENT_TYPE ": create request body with history length = "
        + std::to_string(m_history.size());
    stdLogger.Debug(msg.c_str());
    requestBody["model"] = m_model;
    requestBody["messages"] = messagesArray;

    // tool calling
    if (!m_tools.isEmpty()) {
        requestBody["tools"] = m_tools;
    }

    if (stream) {
        requestBody["stream"] = true;
    }

    stdLogger.Verbose(QJsonDocument(requestBody).toJson().toStdString());
    return QJsonDocument(requestBody).toJson();
}

Client::Message Client::addUserMessage(const QString& message) {
    Message msg;
    msg.id = generateMsgId();
    msg.role = "user";
    msg.content = message;
    msg.tool_call_id = "";
    msg.tool_calls = QJsonArray();
    m_history.append(msg);
    return msg;
}

Client::Message Client::createAssistantMessage(const QString& message, QJsonArray *tool_calls) {
    Message msg;
    msg.id = generateMsgId();
    msg.role = "assistant";
    if (tool_calls) {
        msg.tool_calls = *tool_calls;
        msg.content = "";
        msg.tool_call_id = "";
    } else {
        msg.content = message;
        msg.tool_calls = QJsonArray();
        msg.tool_call_id = "";
    }
    return msg;
}

void Client::addAssistantMessage(Message &msg) {
    // 注意：如果有 think 块则不放入历史记录中
    QString convMsg;
    if (m_thinking) {
        convMsg = Client::removeTags("think", msg.content);
    } else {
        convMsg = msg.content;
    }
    msg.content = convMsg;
    m_history.append(msg);
}

Client::Message Client::addToolMessage(const QString& tool_call_id, const QString& content) {
    Message msg;
    msg.id = generateMsgId();
    msg.role = "tool";
    msg.content = content;
    msg.tool_call_id = tool_call_id;
    msg.tool_calls = QJsonArray();
    m_history.append(msg);
    return msg;
}

std::pair<Client::Message, bool> Client::processReply(QNetworkReply* reply) {
    // create empty msg without tools
    Client::Message msg = createAssistantMessage("");

    if (reply->error() != QNetworkReply::NoError) {
        stdLogger.Exception(CLIENT_TYPE ": server response error: "
            + reply->errorString().toStdString());
        msg.content = reply->errorString();
        return {msg, false};
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull()) {
        stdLogger.Exception(CLIENT_TYPE ": client side error: invalid JSON response (null)");
        msg.content = "Invalid JSON response (null)";
        return {msg, false};
    }

    stdLogger.Verbose(doc.toJson().toStdString());
    stdLogger.Info(CLIENT_TYPE ": client received response");

    const QJsonObject root = doc.object();
    const QString finishReason = root["choices"].toArray().first().toObject()["finish_reason"].toString();
    // handle tool calling
    if (finishReason == "tool_calls") {
        msg.tool_calls = root["choices"].toArray().first()
                        .toObject()["message"].toObject()["tool_calls"].toArray();
        // format
        msg.tool_calls = this->formatToolCalls2OAIFormat(msg.tool_calls);
    } else {
        // normal response
        const QString content = root["choices"].toArray().first()
                           .toObject()["message"].toObject()["content"].toString();
    
        if (content.isEmpty()) {
            stdLogger.Exception(CLIENT_TYPE ": client side error: empty or malformat response content\n"
                + doc.toJson().toStdString());
            msg.content = "Empty response content";
            return {msg, false};
        }
    }
    
    return {msg, true};
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
    stdLogger.Verbose(CLIENT_TYPE ": stream event received. Ready to process");
    stdLogger.Verbose(eventData.toStdString());
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

            auto& context = m_streamContexts[reply];

            // detect tool calling
            if (delta.contains("tool_calls")) {
                context.hasToolCalls = true;
                // 这里需要处理流式 tool_calls 增量
                this->mergeToolCallsStreamDeltaTo(delta["tool_calls"].toArray(), &context.tool_calls);
            }
            else if (delta.contains("content")) {
                QString chunk = delta["content"].toString();
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
    std::pair<Message, bool> rp = processReply(reply);
    Message replyMsg = rp.first;
    bool success = rp.second;
    reply->deleteLater();
    
    if (success) {
        // 添加到历史记录
        addAssistantMessage(replyMsg);
        
        if (replyMsg.role == "assistant" && !replyMsg.tool_calls.isEmpty()) {
            emit toolCallsReceived(replyMsg.tool_calls);
        } else {
            emit asyncResponseReceived(replyMsg.content);
        }
    } else {
        emit errorOccurred(replyMsg.content);
    }
}

void Client::handleAsyncTimeout(QNetworkReply* reply, Message relatedMsg) {
    stdLogger.Warning(CLIENT_TYPE ": user message timeout (async)");

    // 检查是否需要回滚消息
    if (relatedMsg.role == "tool") {
        // 如果模型回复超时时，上一条消息是工具调用的结果，那么就不需要回滚，因为下次用户再追问一下就可以了
        // do nothing here
        stdLogger.Warning("llm timeout when async responding to tool calling result. Skipped unrolling message");
    } else {
        m_history.removeOne(relatedMsg);
    }

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
    
    // 处理缓冲区中剩余数据
    if (!context.buffer.isEmpty()) {
        processStreamEvent(reply, context.buffer);
    }

    // 清理
    m_pendingReplies.remove(reply);
    reply->deleteLater();
    
    Client::Message msg;
    // 如果这个 stream 是工具调用请求，那么一定只有一个 SSE 事件，并且不会有 content（accumulatedResponse）
    if (context.hasToolCalls) {
        // assert(context.accumulatedResponse.isEmpty());
        context.tool_calls = this->formatToolCalls2OAIFormat(context.tool_calls);
        
        msg = createAssistantMessage("", &context.tool_calls);
        addAssistantMessage(msg);
        QJsonDocument doc(context.tool_calls);
        QString docStr = doc.toJson();
        stdLogger.Debug("tool calls detected: " + docStr.toStdString());
        // 单次工具调用请求结束，控制流交给调用方给到 MCP client 或者其他管理 tool 的程序
        emit toolCallsReceived(context.tool_calls);
    }
    // 大模型正常回复非空字符串，则添加到历史记录
    else if (!context.accumulatedResponse.isEmpty()) {
        msg = createAssistantMessage(context.accumulatedResponse);
        addAssistantMessage(msg);
        // 正常流式回复结束
        emit streamFinished();
    }
    else {
        stdLogger.Warning("LLM return an empty string! Why?");
    }
}

void Client::handleStreamTimeout(QNetworkReply *reply) {
    if (!m_streamContexts.contains(reply)) {
        stdLogger.Warning(CLIENT_TYPE ": received a stream timeout event but not recognized");
        return;
    }
    
    auto context = m_streamContexts.take(reply);
    context.timeoutTimer->deleteLater();
    
    // 如果超时时已经有部分响应了，就添加部分响应到历史记录
    if (!context.accumulatedResponse.isEmpty()) {
        Client::Message msg = createAssistantMessage(context.accumulatedResponse);
        addAssistantMessage(msg);
    } else if (context.lastMessage.role == "tool") {
        // 如果模型回复超时时，上一条消息是工具调用的结果，那么就不需要回滚，因为下次用户再追问一下就可以了
        // do nothing here
        stdLogger.Warning("llm timeout when stream responding to tool calling result. Skipped unrolling message");
    } else {
        // 没有任何响应时回滚用户消息
        m_history.removeOne(context.lastMessage);
    }
    
    // 清理
    m_pendingReplies.remove(reply);
    reply->abort();
    reply->deleteLater();
    
    emit errorOccurred("Stream Request timeout");
}

void Client::abortAllRequests() {
    stdLogger.Info(CLIENT_TYPE ": client is aborting all the handling requests");
    for (QNetworkReply* reply : m_pendingReplies) {
        reply->abort();
        reply->deleteLater();
    }
    m_pendingReplies.clear();
}

void Client::mergeToolCallsStreamDeltaTo(QJsonArray partialToolCalls, QJsonArray *dst) {
    for (const auto &delta_tool_call: partialToolCalls) {
        QJsonObject call_obj = delta_tool_call.toObject();
        int current_update_index;
        if (!call_obj.contains("index")) {
            stdLogger.Warning("not OpenAI tool stream format: no tool index in stream. Regarded as 0");
            current_update_index = 0;
        } else {
            current_update_index = call_obj["index"].toInt();
        }
        // expand dst array if necessary
        int current_dst_len = dst->count();
        if (current_dst_len < current_update_index + 1) {
            for (int i = 0; i < current_update_index + 1 - current_dst_len; ++i) {
                dst->append(QJsonObject());
            }
        }
        QJsonObject dst_call_obj = (*dst)[current_update_index].toObject();
        if (call_obj.contains("type")) {
            dst_call_obj["type"] = dst_call_obj["type"].toString("") + call_obj["type"].toString();
        }
        if (call_obj.contains("id")) {
            dst_call_obj["id"] = dst_call_obj["id"].toString("") + call_obj["id"].toString();
        }
        if (call_obj.contains("function")) {
            QJsonObject func_obj = call_obj["function"].toObject();
            QJsonObject dst_func_obj = dst_call_obj["function"].toObject();
            if (func_obj.contains("name")) {
                dst_func_obj["name"] = dst_func_obj["name"].toString("") + func_obj["name"].toString();
            }
            // 注意，arguments 可以以字符串流式传递，但实质上是 JSON 对象
            if (func_obj.contains("arguments")) {
                dst_func_obj["arguments"] = dst_func_obj["arguments"].toString("") + func_obj["arguments"].toString();
            }
            dst_call_obj["function"] = dst_func_obj;
        }
        (*dst)[current_update_index] = dst_call_obj;
    }
}

QJsonArray Client::formatToolCalls2OAIFormat(QJsonArray tool_calls) {
    int tool_calls_cnt = tool_calls.count();
    if (tool_calls_cnt == 0) return tool_calls;
    // check format
    bool no_type = true;
    bool no_tool_call_id = true;
    if (tool_calls[0].toObject().contains("type")) {
        no_type = false;
        if (tool_calls[0].toObject()["type"] != "function") {
            // 错误的格式，抛给下游处理
            stdLogger.Exception("invalid tool calls from llm: unsupported tool type other than function");
            return tool_calls;
        }
    }
    if (tool_calls[0].toObject().contains("id")) {
        no_tool_call_id = false;
    }
    if (!tool_calls[0].toObject()["function"].toObject().contains("name")) {
        // 错误的格式，抛给下游处理
        stdLogger.Exception("invalid tool calls from llm: no function name");
        return tool_calls;
    }
    for (int i = 0; i < tool_calls_cnt; ++i) {
        QJsonObject call_obj = tool_calls[i].toObject();
        if (no_type) {
            call_obj["type"] = "function";
        }
        if (no_tool_call_id) {
            call_obj["id"] = QString::number(generateMsgId());
        }
        tool_calls[i] = call_obj;
    }
    return tool_calls;
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
void Client::setTools(const QJsonArray &tools) {
    std::string msg = CLIENT_TYPE ": client tools is set to array:len=" + std::to_string(tools.count());
    stdLogger.Debug(msg);
    m_tools = tools;
}
