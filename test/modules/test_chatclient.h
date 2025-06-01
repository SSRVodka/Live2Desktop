#pragma once

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtTest/QTest>
#include <queue>
#include <mutex>

#include "modules/chat/openai_client.h"

using namespace Chat;

class TestChatClient : public QObject {
    Q_OBJECT
public:

    static constexpr int MOC_SERVER_PORT = 26565;

    struct _server_res_t {
        QString text;
        bool valid;
    };

    TestChatClient();
    ~TestChatClient() {}

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testNormal();
    void testInvalidRequest();
    void testInvalidResponse();
    void testHistory();
    void testTimeout();

    // TODO
    // void testStream();

protected slots:
    void recvResp(const QString& response);
    void recvError(const QString& error);

private:

    void submit_and_check(
        bool wait_for_error = false,
        bool check_str = false,
        QString compare_str = ""
    );

    std::mutex msg_mutex;
    std::queue<_server_res_t> msg_queue;
    static constexpr int TEST_UNIT_TIMEOUT = 10000; // ms
    static constexpr int TEST_RECORD_TIME = 10;  // s

    static constexpr int TESTLOG_BUFSIZE = 4096;
    char logbuf[TESTLOG_BUFSIZE];

    Client *client;
    Client::chat_params_t params;
    QString endpoint;
    QProcess mock_server;
};
