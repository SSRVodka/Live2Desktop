
#include "modules/stt/client.h"

#include "utils/logger.h"

namespace STT {

int Client::POLLING_INTERVAL = 500 /* milli-seconds */;


Client::Client(const std::string &host, uint16_t port)
    :proxyWorker(host, port), taskID(-1) {
    
    this->pollingTimer.setInterval(Client::POLLING_INTERVAL);
    connect(&this->proxyWorker, SIGNAL(started()), &this->pollingTimer, SLOT(start()));
    connect(&this->pollingTimer, SIGNAL(timeout()), this, SLOT(polling()));
    this->proxyWorker.start();
}

Client::~Client() {
    this->proxyWorker.terminateGracefully();
    stdLogger.Info("Client: waiting for worker thread to terminate...");
    this->proxyWorker.wait();
}

void Client::sendMp3(const std::string &mp3file, const whisper_params &params) {
    // TODO: convert
}
void Client::sendWav(const std::string &wavfile, const whisper_params &params) {

    Worker::State curState = this->proxyWorker.getState();
    const char *errmsg;
    switch (curState) {
    case Worker::State::BUSY:
        errmsg = "STT server busy. Please wait";
        stdLogger.Warning(errmsg);
        emit replyArrived(false, QString(errmsg));
        return;
    case Worker::State::OFFLINE:
        errmsg = "STT worker offline. Try again later";
        stdLogger.Warning(errmsg);
        emit replyArrived(false, QString(errmsg));
        return;
    case Worker::State::IDLE:
        break;
    }

    this->taskID = this->proxyWorker.submitTask(wavfile, params);
    if (this->taskID < 0) {
        const char errmsg[] = "Client: STT task submission failure";
        stdLogger.Warning(errmsg);
        emit replyArrived(false, QString(errmsg));
    } else {
        std::string msg = "Client: get task ID: " + std::to_string(this->taskID);
        stdLogger.Info(msg.c_str());
        stdLogger.Info("Client: polling for response...");
    }
}

void Client::polling() {
    Worker::State curState = this->proxyWorker.getState();
    std::string msg = std::string("Client: polling state: ") + (
        curState == Worker::State::BUSY ? "BUSY" : (
        curState == Worker::State::IDLE ? "IDLE" : "OFFLINE"
    ));
    stdLogger.Debug(msg.c_str());
    
    const char *errmsg;
    switch (curState) {
    case Worker::State::IDLE:
        if (this->taskID == this->proxyWorker.getCurrentTaskID()) {
            msg = this->proxyWorker.getCurrentResult();
            emit replyArrived(true, QString(msg.c_str()));
            // invalidate task ID until next time we send
            this->taskID = -1;
        }
        break;
    case Worker::State::OFFLINE:
        if (this->taskID > 0) {
            errmsg = "Worker offline. Task progress lost!";
            stdLogger.Warning(errmsg);
            emit replyArrived(false, QString(errmsg));
            this->taskID = -1;
        }
        break;
    case Worker::State::BUSY:
        break;
    }
}

int Worker::MAX_RETRY_TIMES = 5;
int Worker::RETRY_INTERVAL = 1000 /* milli-seconds */;
int Worker::HEARTBEAT_INTERVAL = 1000 /* milli-seconds */;

Worker::Worker(const std::string &host, uint16_t port, QObject *parent)
    : currentState(State::OFFLINE),
    stt_client(host, port),
    currentID(0) {

    this->termFlag.store(false);
    stdLogger.Info("STT Worker initializing...");
}

Worker::~Worker() {}

void Worker::terminateGracefully() {
    this->termFlag.store(true);
}

bool Worker::heartbeat() {
    auto hbRes = stt_client.Get("/heartbeat");
    
    bool healthy = (hbRes && hbRes->status == httplib::OK_200);
    this->currentState = healthy
        ? (this->currentState == State::BUSY ? State::BUSY : State::IDLE)
        : State::OFFLINE;
    std::string msg = "Worker ping STT server [" + stt_client.host()
        + ":" + std::to_string(stt_client.port()) + std::string("]: ")
        + (healthy ? "Healthy" : "Offline");
    stdLogger.Debug(msg.c_str());
    return healthy;
}

int64_t Worker::submitTask(const std::string &wavfile, const whisper_params &params) {
    std::string msg = "Task info: {wav: '" + wavfile + "'}";
    if (this->currentState == State::OFFLINE) {
        stdLogger.Warning("Worker offline. Task rejected.");
        stdLogger.Warning(msg.c_str());
        return -1;
    }
    if (this->currentState == State::BUSY) {
        stdLogger.Warning("Worker(singleton) busy. Task rejected.");
        stdLogger.Warning(msg.c_str());
        return -1;
    }
    this->currentFile = wavfile;
    this->currentParams = params;
    this->currentState = State::BUSY;
    return this->currentID + 1;
}

void Worker::executeTask() {
    this->currentResult = "";
    this->resultValid = false;
    int retryTimes = 1;
    std::string msg = "Worker: submit '" + this->currentFile + "' to inference";
    httplib::MultipartFormDataItems formTable = {
        {"file", "", this->currentFile, "audio/wav"},
        {"offset_t", std::to_string(this->currentParams.offset_t_ms), "", ""},
        {"offset_n", std::to_string(this->currentParams.offset_n), "", ""},
        {"duration", std::to_string(this->currentParams.duration_ms), "", ""},
        {"max_content", std::to_string(this->currentParams.max_context), "", ""},
        {"max_len", std::to_string(this->currentParams.max_len), "", ""},
        {"best_of", std::to_string(this->currentParams.best_of), "", ""},
        {"beam_size", std::to_string(this->currentParams.beam_size), "", ""},
        {"audio_ctx", std::to_string(this->currentParams.audio_ctx), "", ""},
        {"word_thold", std::to_string(this->currentParams.word_thold), "", ""},
        {"entropy_thold", std::to_string(this->currentParams.entropy_thold), "", ""},
        {"logprob_thold", std::to_string(this->currentParams.logprob_thold), "", ""},
        {"debug_mode", std::to_string(this->currentParams.debug_mode), "", ""},
        {"translate", std::to_string(this->currentParams.translate), "", ""},
        {"diarize", std::to_string(this->currentParams.diarize), "", ""},
        {"tinydiarize", std::to_string(this->currentParams.tinydiarize), "", ""},
        {"split_on_word", std::to_string(this->currentParams.split_on_word), "", ""},
        {"no_timestamps", std::to_string(this->currentParams.no_timestamps), "", ""},
        {"language", this->currentParams.language, "", ""},
        {"detect_language", std::to_string(this->currentParams.detect_language), "", ""},
        {"prompt", this->currentParams.prompt, "", ""},
        {"response_format", this->currentParams.response_format, "", ""},
        {"temperature", std::to_string(this->currentParams.temperature), "", ""},
        {"temperature_inc", std::to_string(this->currentParams.temperature_inc), "", ""},
        {"suppress_nst", std::to_string(this->currentParams.suppress_nst), "", ""},
    };
    while (retryTimes > Worker::MAX_RETRY_TIMES) {
        if (!this->heartbeat()) {
            std::string errmsg = "Worker offline. Retry ("
                + std::to_string(retryTimes) + "/"
                + std::to_string(Worker::MAX_RETRY_TIMES) + ")";
            stdLogger.Exception(errmsg.c_str());
            this->msleep(Worker::RETRY_INTERVAL);
            ++retryTimes;
            continue;
        }
        stdLogger.Info(msg.c_str());
        auto res = stt_client.Post("/inference", formTable);
        if (res && res->status == httplib::OK_200) {
            std::string ackmsg = "Worker: retrieve result successfully: " + res->body;
            stdLogger.Info(ackmsg.c_str());
            this->currentResult = res->body;
            this->resultValid = true;
            break;
        } else {
            std::string errmsg = "Worker: failed to obtain result from server (code: "
                + (res ? std::to_string(res->status) : "NULL") + "). Retry: ("
                + std::to_string(retryTimes) + "/"
                + std::to_string(Worker::MAX_RETRY_TIMES) + ")";
            stdLogger.Exception(errmsg.c_str());
        }
        ++retryTimes;
    }
}

void Worker::run() {
    while (!this->termFlag.load()) {
        if (this->currentState == State::BUSY) {
            this->executeTask();
            this->currentID++;
            this->currentState = State::IDLE;
        }
        this->heartbeat();
        this->msleep(Worker::HEARTBEAT_INTERVAL);
    }
}


};
