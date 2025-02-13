#pragma once

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QTcpServer>

namespace Chat {

class Server: public QObject {
	Q_OBJECT

public:
	Server(QObject *parent);
	~Server();
	bool startServer(QString, quint16);		// This is the method that is called when the server starts
	void sendMessage(QString);				// This is the method that is called when the server sends a message

protected slots:
	void connection();						// This is the slot that is called when the server receives a connection
	void receiveMessage();					// This is the slot that is called when the server receives a message

private:
	QTcpServer *server;
	QTcpSocket* socket;

signals:
	void messageReceived(QString, int);		// This is the signal that is emitted when the server receives a message
};

};
