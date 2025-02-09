#include <QtNetwork/QTcpSocket>
#include <QtCore/QByteArray>

#include "modules/chat/client.h"

namespace Chat {

Client::Client(QObject *parent)
	: QObject(parent) {
	socket = new QTcpSocket(this);
}

Client::~Client() {
	delete socket;
}

bool Client::connectToServer(QString ip, quint16 port) {
	// Connect to the server.
	socket->connectToHost(ip, port);

	if (socket->waitForConnected()) {
		// When the socket has data to read, call the receiveMessage() slot.
		connect(socket, SIGNAL(readyRead()), this, SLOT(receiveMessage()));
		return true;
	}
	else {
		return false;
	}
}

// This is the slot that is called when the socket has data to read.
void Client::receiveMessage() {
	QByteArray data = socket->readAll();
	QString message = QString::fromUtf8(data);

	emit messageReceived(message, 0);
}

// This is the method that is called when the socket has data to send.
void Client::sendMessage(QString message) {
	QByteArray data = message.toUtf8();
	socket->write(data);
}

};