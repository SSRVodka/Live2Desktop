/**
 * @file downloader.h
 * @brief Network downloading helpers
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */

#pragma once

#include <QtCore/QFile>
#include <QtWidgets/QProgressDialog>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>


class FileDownloader : public QObject {
    Q_OBJECT
public:
    explicit FileDownloader(QObject *parent = nullptr);

    void downloadAsync(const QUrl &url, const QString &savePath);
    bool downloadSync(const QUrl &url, const QString &savePath, bool progressGUI = false);

signals:
    void downloadFinished(bool success, QString msg);
private slots:
    void onReadyRead();
    void onProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onFinished();

private:

    void setupProgressDialog();
    void updateProgressDialog(qint64 recv, qint64 total);
    void cleanProgressDialog();

    // 文件大小格式化显示
    static QString formatSize(qint64 bytes) {
        if(bytes < 1024) return QString::number(bytes) + " B";
        if(bytes < 1024*1024) return QString::number(bytes/1024.0, 'f', 1) + " KB";
        return QString::number(bytes/(1024.0*1024.0), 'f', 1) + " MB";
    }

    QNetworkAccessManager manager;
    QNetworkReply *reply = nullptr;
    QFile file;
    QProgressDialog *progressDialog = nullptr;
};