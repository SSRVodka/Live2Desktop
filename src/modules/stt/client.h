/**
 * @file client.h
 * @brief Speech to text worker & client class.
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */

#pragma once

#include <atomic>
#include <httplib.cpp/httplib.h>
#include <sv.cpp/sense-voice/include/asr_handler.hpp>

#include <QtCore/QTimer>
#include <QtCore/QThread>


namespace STT {

class Worker : public QObject {
    Q_OBJECT
public:
    Worker(ASRHandler* server) : server(server) {}

public slots:
    void process(ASRHandler::task_id_t taskId, ASRHandler::asr_params params);

signals:
    void resultReady(ASRHandler::asr_result result);

private:
    ASRHandler* server;
};


/**
 * @brief Speech to text client for sending audio and retrieving text
 */
class Client: public QObject {
    Q_OBJECT
public:

    // local connection
    Client(QObject *parent = nullptr);
    // TODO: remote client
	Client(const std::string &host, uint16_t port);
	~Client();

    void sendWav(const ASRHandler::asr_params &params);

signals:
    /** 
     * Triggered when receive the text response from the STT server
     */
    void replyArrived(bool valid, const QString &transcribed_text);

    /**
     * [Used Internally] Triggered when we start the ASRServer asynchronously (using worker)
     */
     void startProcessing(ASRHandler::task_id_t taskId, ASRHandler::asr_params params);

protected slots:
    void handleResult(ASRHandler::asr_result result);

private:
    std::unique_ptr<QThread> workerThread;
    Worker *worker;
    ASRServer *server;
    // < 0 for invalid/failure
    ASRHandler::task_id_t taskID;
};

};


