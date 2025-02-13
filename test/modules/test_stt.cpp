
#include "utils/logger.h"
#include "test_stt.h"

using namespace STT;

void TestSTT::testNoServer() {
    connect(
        &this->client, SIGNAL(replyArrived(bool, QString)),
        this, SLOT(onReplyArrived_testNoServer(bool, QString))
    );
    whisper_params params;
    client.sendWav("123", params);
    disconnect(
        &this->client, SIGNAL(replyArrived(bool, QString)),
        this, SLOT(onReplyArrived_testNoServer(bool, QString))
    );
}

void TestSTT::onReplyArrived_testNoServer(bool valid, QString transcribed_text) {
    snprintf(this->logbuf, TestSTT::TESTLOG_BUFSIZE, "receive: valid=%d, text='%s'",
        valid, transcribed_text.toStdString().c_str());
    this->pass += !valid;
    stdLogger.Test(this->logbuf);
    termFlag = true;
}

void TestSTT::report() {
    snprintf(this->logbuf, TestSTT::TESTLOG_BUFSIZE, "TestSTT summary: %d/%d (pass/total)\n",
        this->pass, this->total);
    stdLogger.Test(this->logbuf);
}

int main() {
    TestSTT test;
    test.testNoServer();
    while (!test.termFlag) {
        sleep(1);
    }
    test.report();
}
