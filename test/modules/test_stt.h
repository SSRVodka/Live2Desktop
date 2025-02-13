#pragma once

#include <QtCore/QObject>
#include <unistd.h>
#include "modules/stt/client.h"

using namespace STT;

class TestSTT: public QObject {
    Q_OBJECT
public:
    bool termFlag;

    TestSTT(): termFlag(false), pass(0), client("127.0.0.1", 12345) {}
    ~TestSTT() {}

    void testNoServer();
    void report();
protected slots:
    void onReplyArrived_testNoServer(bool valid, QString transcribed_text);
private:

    static constexpr int TESTLOG_BUFSIZE = 4096;
    char logbuf[TESTLOG_BUFSIZE];
    int pass;
    static constexpr int total = 1;
    Client client;
};