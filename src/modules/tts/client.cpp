
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkRequest>

#include "modules/tts/client.h"

#include "utils/consts.h"
#include "utils/logger.h"

using namespace TTS;

#define CLIENT_TYPE "TTS Client "

Client::Client(QObject *parent): QObject(parent) {}

Client::~Client() {
    cleanup();
}

void Client::generateSpeech(const tts_params_t &params, const QString& input, const QString& filePath) {
    cleanup();

    m_outputFile.reset(new QFile(filePath));
    if (!m_outputFile->open(QIODevice::WriteOnly)) {
        QString errmsg = QString("file open error: %1").arg(m_outputFile->errorString());
        std::string conv = CLIENT_TYPE + errmsg.toStdString();
        stdLogger.Exception(conv.c_str());
        emit finished(false, errmsg);
        return;
    }

    QNetworkRequest request(QString::fromStdString(params.server_url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!params.api_key.empty()) {
        QString token = QString("Bearer %1").arg(params.api_key.c_str());
        request.setRawHeader("Authorization", token.toUtf8());
    }

    QJsonObject json;
    json["model"] = params.model.c_str();
    json["voice"] = params.voice.c_str();
    json["input"] = input;
    json["response_format"] = MODEL_CAP_CONTAINER_FORMAT;

    m_currentReply = m_manager.post(request, QJsonDocument(json).toJson());

    QObject::connect(m_currentReply, &QNetworkReply::readyRead, [this]() {
        stdLogger.Debug(CLIENT_TYPE "stream read from TTS server");
        m_outputFile->write(m_currentReply->readAll());
    });

    QObject::connect(m_currentReply, &QNetworkReply::finished, [this]() {
        if (m_currentReply->error() == QNetworkReply::NoError) {
            m_outputFile->close();
            QString msg = CLIENT_TYPE "save data to " + m_outputFile->fileName();
            std::string conv = msg.toStdString();
            stdLogger.Info(conv.c_str());
            emit finished(true, "OK");
        } else {
            handleError(m_currentReply->errorString());
        }
        cleanup();
    });
}

void Client::cleanup()
{
    if (m_currentReply) {
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    if (m_outputFile && m_outputFile->isOpen()) {
        m_outputFile->close();
    }
}

void Client::handleError(const QString& error)
{
    if (m_outputFile) {
        m_outputFile->remove();  // 删除不完整的文件
        stdLogger.Warning("cleaning broken files");
    }
    QString msg = QString("Request failed: %1").arg(error);
    std::string conv = CLIENT_TYPE + msg.toStdString();
    stdLogger.Exception(conv.c_str());
    emit finished(false, msg);
}
