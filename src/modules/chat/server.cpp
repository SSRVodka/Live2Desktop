
#include "modules/chat/server.h"

namespace Chat {

Server::Server(QObject *parent)
	: QObject(parent) {
	server = new QTcpServer(this);
	socket = new QTcpSocket(this);
}

Server::~Server() {
	delete socket;
}

bool Server::startServer(QString ipAddress, quint16 port) {
	// Start the server
	QHostAddress ip(ipAddress);
	if (!server->listen(ip, port)) {
		return false;
	}
	// Connect the server to the connection slot
	connect(server, SIGNAL(newConnection()), this, SLOT(connection()));
	return true;
}

// This is the slot that is called when the server receives a connection
void Server::connection() {
	socket = server->nextPendingConnection();
	// Connect the socket to the receiveMessage slot
	connect(socket, SIGNAL(readyRead()), this, SLOT(receiveMessage()));
}

// This is the slot that is called when the server receives a message
void Server::receiveMessage() {
	QByteArray data = socket->readAll();
	QString message = QString::fromUtf8(data);
	emit messageReceived(message, 0);
}

// This is the method that is called when the server sends a message
void Server::sendMessage(QString message) {
	QByteArray data = message.toUtf8();
	socket->write(data);
}

};
