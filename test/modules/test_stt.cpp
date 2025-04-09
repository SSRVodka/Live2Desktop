
#include <QtTest/QSignalSpy>
#include "test_stt.h"

#include "modules/audio/audio_recorder.h"
#include "utils/logger.h"

#define TEST_MODEL "models/sv-small-q3_k.gguf"
#define TEST_WAV "test_data/test.wav"

TestSTT::TestSTT() {}

void TestSTT::initTestCase() {
    chdir(QCoreApplication::applicationDirPath().toStdString().c_str());
    // check test file
    QVERIFY2(fileExists(TEST_MODEL), "test model " TEST_MODEL " not found");
    QVERIFY2(fileExists(TEST_WAV), "test wav " TEST_WAV " not found");

    handler = new AudioHandler;
    client = new Client("127.0.0.1", 12345);
    
    connect(
        client, SIGNAL(replyArrived(bool, QString)),
        this, SLOT(onReplyArrived(bool, QString))
    );
}

void TestSTT::cleanupTestCase() {
    delete client;
    delete handler;
}

void TestSTT::testNoServer() {
    ASRServer::asr_params params;
    params.fname_inp.emplace_back("invalid.wav");
    
    // 使用 QSignalSpy 监听信号
    QSignalSpy spy(client, &Client::replyArrived);
    
    client->sendWav(params);
    
    QVERIFY(spy.wait(TEST_UNIT_TIMEOUT));  // 自动处理事件循环
    
    // 检查结果
    msg_mutex.lock();
    QVERIFY(!msg_queue.empty());
    _server_res_t res = msg_queue.front();
    QCOMPARE(res.valid, false);
    msg_queue.pop();
    msg_mutex.unlock();
}

void TestSTT::testNormal() {
    ASRServer::asr_params params;
    params.fname_inp.emplace_back(TEST_WAV);
    params.model = TEST_MODEL;
    params.language = "zh";

    QSignalSpy spy(client, &Client::replyArrived);

    client->sendWav(params);
    
    QVERIFY(spy.wait(TEST_UNIT_TIMEOUT));
    
    msg_mutex.lock();
    QVERIFY(!msg_queue.empty());
    _server_res_t res = msg_queue.front();
    QCOMPARE(res.valid, true);
    QVERIFY(!res.text.isEmpty());
    msg_queue.pop();
    msg_mutex.unlock();
}

void TestSTT::testRecorder() {
    this->handler->get_recorder_unsafe_ptr()->record();
    QThread::currentThread()->sleep(TEST_RECORD_TIME);
    QString audio_file = this->handler->get_recorder_unsafe_ptr()->record_stop();
    QVERIFY(!audio_file.isEmpty());
    std::string fn = audio_file.toStdString();

    ASRServer::asr_params params;
    params.fname_inp.emplace_back(fn);
    params.model = TEST_MODEL;

    QSignalSpy spy(client, &Client::replyArrived);

    client->sendWav(params);
    
    QVERIFY(spy.wait(TEST_UNIT_TIMEOUT));
    
    msg_mutex.lock();
    QVERIFY(!msg_queue.empty());
    _server_res_t res = msg_queue.front();
    QCOMPARE(res.valid, true);
    msg_queue.pop();
    msg_mutex.unlock();
}

void TestSTT::onReplyArrived(bool valid, QString transcribed_text) {
    snprintf(logbuf, TESTLOG_BUFSIZE, "receive: valid=%d, text='%s'",
            valid, transcribed_text.toStdString().c_str());
    stdLogger.Test(logbuf);
    
    _server_res_t res;
    res.text = transcribed_text;
    res.valid = valid;
    std::lock_guard<std::mutex> lock(msg_mutex);
    msg_queue.push(res);
}

QTEST_MAIN(TestSTT)
