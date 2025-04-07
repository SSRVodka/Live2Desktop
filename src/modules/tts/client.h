#pragma once

#include <QtCore/QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "utils/consts.h"

namespace TTS {

class Client : public QObject {
    Q_OBJECT
public:

    

    explicit Client(QObject *parent = nullptr);
    ~Client();
    void generateSpeech(const tts_params_t &params, const QString& input, const QString& filePath);

signals:
    void finished(bool success, const QString& error);

private:
    QNetworkAccessManager m_manager;
    QNetworkReply* m_currentReply = nullptr;
    QScopedPointer<QFile> m_outputFile;

    void cleanup();
    void handleError(const QString& error);
};


};
