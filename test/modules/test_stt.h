#pragma once

#include <QtCore/QObject>
#include <QtTest/QTest>
#include <queue>
#include <mutex>

#include "modules/audio/audio_handler.h"
#include "modules/stt/client.h"

using namespace STT;

class TestSTT : public QObject {
    Q_OBJECT
public:
    TestSTT();
    ~TestSTT() {}

    struct _server_res_t {
        QString text;
        bool valid;
    };

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testNoServer();
    void testNormal();
    void testRecorder();

protected slots:
    void onReplyArrived(bool valid, QString transcribed_text);

private:
    std::queue<_server_res_t> msg_queue;
    std::mutex msg_mutex;
    static constexpr int TEST_UNIT_TIMEOUT = 10000; // ms
    static constexpr int TEST_RECORD_TIME = 10;  // s

    static constexpr int TESTLOG_BUFSIZE = 4096;
    char logbuf[TESTLOG_BUFSIZE];
    
    Client *client;
    AudioHandler *handler;
};
