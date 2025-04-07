
#include "modules/stt/client.h"

#include "utils/logger.h"

namespace STT {

#define CLIENT_TYPE "STT Client"

Client::Client(QObject *parent): QObject(parent), taskID(-1) {
    server = new ASRServer;
    workerThread = std::make_unique<QThread>();
    worker = new Worker(server);
    worker->moveToThread(workerThread.get());

    connect(this, &Client::startProcessing, worker, &Worker::process, Qt::QueuedConnection);
    connect(worker, &Worker::resultReady, this, &Client::handleResult);
    workerThread->start();
    stdLogger.Info(CLIENT_TYPE ": worker thread started");
}
Client::Client(const std::string &host, uint16_t port): Client() {}

Client::~Client() {
    stdLogger.Debug(CLIENT_TYPE ": now prepared to shutdown worker thread");
    workerThread->quit();
    stdLogger.Info(CLIENT_TYPE ": waiting for the worker to stop...");
    workerThread->wait();
    stdLogger.Info(CLIENT_TYPE ": worker thread stopped");
    delete worker;
    delete server;
}

void Client::sendWav(const ASRHandler::asr_params &params) {
    ++this->taskID;
    std::string msg = CLIENT_TYPE ": worker started with task ID: "
        + std::to_string(taskID);
    stdLogger.Info(msg.c_str());
    emit startProcessing(taskID, params);
}

void Client::handleResult(ASRHandler::asr_result result) {
    std::string msg;
    if (result.request_id == this->taskID) {
        msg = CLIENT_TYPE ": [task ID " + std::to_string(result.request_id)
            + "] receive result from server '" + result.text + "'";
        stdLogger.Info(msg.c_str());
        emit replyArrived(true, QString::fromStdString(result.text));
    } else {
        // TODO: add reason
        msg = CLIENT_TYPE ": [task ID " + std::to_string(result.request_id)
            + "] server reply with error (unknown reason)";
        stdLogger.Warning(msg.c_str());
        emit replyArrived(false, QString());
    }
}

// void Client::on_server_output_append(ASRServer::task_id_t, QString) {
// }

void Worker::process(ASRHandler::task_id_t taskId, ASRHandler::asr_params params) {
    ASRHandler::asr_result result = server->handle(taskId, params);
    emit resultReady(result);
}

};
