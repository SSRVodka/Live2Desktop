
#include <QtTest/QSignalSpy>

#include "utils/consts.h"
#include "test_chatclient.h"

TestChatClient::TestChatClient() {
    this->endpoint = QString("http://localhost:%1").arg(MOC_SERVER_PORT);
}

void TestChatClient::submit_and_check(
    bool wait_for_error,
    bool check_str,
    QString compare_str) {
    
    auto signal_ptr = wait_for_error
        ? &Client::errorOccurred
        : &Client::asyncResponseReceived;
    
    QSignalSpy spy(this->client, signal_ptr);

    this->client->sendMessageAsync("hello");

    QVERIFY(spy.wait(TEST_UNIT_TIMEOUT));

    msg_mutex.lock();
    QVERIFY(!msg_queue.empty());
    _server_res_t res = msg_queue.front();
    QCOMPARE(res.valid, !wait_for_error);
    if (check_str) {
        QCOMPARE(res.text, compare_str);
    }
    msg_queue.pop();
    msg_mutex.unlock();
}

void TestChatClient::initTestCase() {
    // chdir && check binary
    chdir(QCoreApplication::applicationDirPath().toStdString().c_str());
    QVERIFY(fileExists("./mock_chatserver"));
    // supress warning
    QStringList args;
    mock_server.start("./mock_chatserver", args);
    QVERIFY(mock_server.waitForStarted(5000));
    QTest::qWait(5000);

    this->client = new Client();
    this->client->setTimeout(TEST_UNIT_TIMEOUT << 1);

    connect(this->client, &Client::asyncResponseReceived,
            this, &TestChatClient::recvResp);
    connect(this->client, &Client::errorOccurred,
            this, &TestChatClient::recvError);
    
}

void TestChatClient::cleanupTestCase() {
    delete this->client;

    mock_server.terminate();
    QVERIFY(mock_server.waitForFinished(3000));
}

void TestChatClient::testNormal() {
    this->params.server_url = this->endpoint + "/v1/chat/completions";
    this->params.api_key = "sk-dwjeifjiedewijepwjiw";
    this->client->setChatParams(this->params);

    this->submit_and_check(false);

    // no API key (when mock server doesn't need it)
    this->params.server_url = this->endpoint + "/v1/nokey";
    this->params.api_key = "";
    this->client->setChatParams(this->params);

    this->submit_and_check(false);
}

void TestChatClient::testInvalidRequest() {
    // no API key (when mock server needed)
    this->params.server_url = this->endpoint + "/v1/chat/completions";
    this->params.api_key = "";
    this->client->setChatParams(this->params);
    this->client->setTimeout(TEST_UNIT_TIMEOUT << 1);

    this->submit_and_check(true);

    // invalid url
    this->params.server_url = "http://localhost:22";
    this->client->setChatParams(this->params);
    this->submit_and_check(true);
    // invalid endpoint
    this->params.server_url = this->endpoint + "/v1/12345";
    this->client->setChatParams(this->params);
    this->submit_and_check(true);
}

void TestChatClient::testInvalidResponse() {
    this->params.server_url = this->endpoint + "/v1/invalid";
    this->client->setChatParams(this->params);
    this->submit_and_check(true);
}

void TestChatClient::testHistory() {
    this->params.server_url = this->endpoint + "/v1/nokey";
    this->params.api_key = "";
    this->client->setChatParams(this->params);
    this->client->clearHistory();
    QVector<Client::Message> vec = this->client->getHistory();
    QVERIFY2(vec.size() == 0,
        "clearHistory() failure: history not empty");

    // simple history
    this->submit_and_check(false);
    vec = this->client->getHistory();
    QCOMPARE(vec.size(), 2);
    QCOMPARE(vec[0].role, "user");
    QCOMPARE(vec[1].role, "assistant");
    this->client->clearHistory();

    // batch
    int msg_cnt = 100;
    for (int i = 0; i < msg_cnt; ++i) {
        // warning: the performance is not good!
        this->submit_and_check(false);
        QCOMPARE(this->client->getHistory().size(), (i + 1) << 1);
    }

    // repeat
    this->client->clearHistory();
    vec = this->client->getHistory();
    QCOMPARE(vec.size(), 0);

    msg_cnt = 200;
    for (int i = 0; i < msg_cnt; ++i) {
        this->submit_and_check(false);
        QCOMPARE(this->client->getHistory().size(), (i + 1) << 1);
    }
}

void TestChatClient::testTimeout() {
    this->params.server_url = this->endpoint + "/v1/timeout";
    this->client->setChatParams(this->params);
    this->client->setTimeout(1000); // 1s
    this->client->clearHistory();
    this->submit_and_check(true, true, "Async request timeout");

    // the client shouldn't keep history if timeout,
    // because caller is responsible to the re-submission
    QCOMPARE(this->client->getHistory().size(), 0);
}

void TestChatClient::testTagsAndBlocks() {
    QString testStr = "Hello! this is a text.<think> I'm <think>ing now... 12345678 </think>\n\n"
        "Here is my code:\n"
        "```python\n"
        "print(\"Hello! world.\")\n"
        "```\n"
        "And HTML:\n"
        "```html\n"
        "<think>Pretend to think</think>"
        "```\n"
        "Done!";
    QString answer = "Hello! this is a text.\n\n"
        "Here is my code:\n\n"
        "And HTML:\n\n"
        "Done!";
    QString res;
    res = Client::removeCodeBlocks(testStr);
    res = Client::removeTags("think", res);
    QCOMPARE(res, answer);
}

void TestChatClient::recvResp(const QString& response) {
    msg_mutex.lock();
    _server_res_t res;
    res.text = response;
    res.valid = true;
    msg_queue.push(res);
    msg_mutex.unlock();
}

void TestChatClient::recvError(const QString& error) {
    msg_mutex.lock();
    _server_res_t res;
    res.text = error;
    res.valid = false;
    msg_queue.push(res);
    msg_mutex.unlock();
}

QTEST_MAIN(TestChatClient)
