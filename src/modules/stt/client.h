#pragma once

#include <atomic>
#include <httplib.cpp/httplib.h>
#include <QtCore/QTimer>
#include <QtCore/QThread>

#include "modules/stt/server.h"

namespace STT {


// TODO: Worker Pool
class Worker: public QThread {

public:
    enum State {
        OFFLINE,
        IDLE,
        BUSY,
    };

    Worker(const std::string &host, uint16_t port, QObject *parent = 0);
    ~Worker();

    State getState() const { return this->currentState; }
    int64_t getCurrentTaskID() const { return this->currentID; }
    bool isResultValid() const { return this->resultValid; }
    std::string getCurrentResult() const { return this->currentResult; }

    // TODO: decouple functions
    // return: incremental task ID. < 0 for failure
    int64_t submitTask(const std::string &wavfile, const whisper_params &params);

    void terminateGracefully();

protected:
    bool heartbeat();
    // sync op
    void executeTask();

    void run() override;

private:

    static int MAX_RETRY_TIMES;
    static int RETRY_INTERVAL;  /* milli-seconds */
    static int HEARTBEAT_INTERVAL;   /* milli-seconds */

    std::atomic<bool> termFlag;
    std::atomic<State> currentState;
    httplib::Client stt_client;

    int64_t currentID;
    std::string currentFile;
    whisper_params currentParams;
    std::string currentResult;
    bool resultValid;
};


class Client: public QObject {
    Q_OBJECT
public:

	Client(const std::string &host, uint16_t port);
	~Client();

    void sendMp3(const std::string &mp3file, const whisper_params &params);
    void sendWav(const std::string &wavfile, const whisper_params &params);

signals:
    /** 
     * Triggered when receive the text response from the STT server
     */
    void replyArrived(bool valid, QString transcribed_text);

protected slots:
    void polling();

private:

    static int POLLING_INTERVAL; /* milli-seconds */

    QTimer pollingTimer;
    Worker proxyWorker;
    // < 0 for invalid/failure
    int64_t taskID;
};

};


