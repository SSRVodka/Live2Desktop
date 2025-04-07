
#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QThread>
#include "network/downloader.h"
#include "utils/logger.h"


FileDownloader::FileDownloader(QObject *parent) : QObject(parent) {}

void FileDownloader::downloadAsync(const QUrl &url, const QString &savePath) {
    QNetworkRequest request(url);
    reply = manager.get(request);

    file.setFileName(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QString msg = QString("file permission failed: ") + file.errorString();
        std::string conv = msg.toStdString();
        stdLogger.Exception(conv.c_str());
        reply->deleteLater();
        return;
    }

    connect(reply, &QNetworkReply::readyRead, this, &FileDownloader::onReadyRead);
    connect(reply, &QNetworkReply::downloadProgress, this, &FileDownloader::onProgress);
    connect(reply, &QNetworkReply::finished, this, &FileDownloader::onFinished);
}

bool FileDownloader::downloadSync(const QUrl &url, const QString &savePath, bool progressGUI) {
    if (progressGUI) this->setupProgressDialog();

    QEventLoop loop;
    bool success = false;
    
    // 连接完成信号到事件循环退出
    connect(this, &FileDownloader::downloadFinished, [&](bool ok, QString msg){
        success = ok;
        if (!ok) {
            std::string conv = msg.toStdString();
            stdLogger.Exception(conv.c_str());
        }
        loop.quit();
    });
    
    downloadAsync(url, savePath);
    if (progressGUI) this->progressDialog->show();
    loop.exec(); // 阻塞等待下载完成

    // clean if dialog exists
    this->cleanProgressDialog();
    return success;
}

void FileDownloader::onReadyRead() {
    if (reply) file.write(reply->readAll());
}

void FileDownloader::onProgress(qint64 bytesReceived, qint64 bytesTotal) {
    std::string msg = "downloaded " + std::to_string(bytesReceived) + " bytes, "
        + std::to_string(bytesTotal) + "bytes in total";
    stdLogger.Debug(msg.c_str());
    // update if dialog exists
    this->updateProgressDialog(bytesReceived, bytesTotal);
}

void FileDownloader::onFinished() {
    // 处理剩余数据并关闭文件
    if (reply->error() == QNetworkReply::NoError) {
        file.write(reply->readAll());
        file.close();
        QString msg = QString("Download finished. File saved to: ") + file.fileName();
        std::string conv = msg.toStdString();
        stdLogger.Info(conv.c_str());
        emit downloadFinished(true, QString("ok"));
    } else {
        file.remove(); // 删除未完成文件
        QString msg = QString("failed to download: ") + reply->errorString();
        std::string conv = msg.toStdString();
        stdLogger.Exception(conv.c_str());
        emit downloadFinished(false, reply->errorString());
    }
    reply->deleteLater();
}

void FileDownloader::setupProgressDialog() {
    this->progressDialog = new QProgressDialog;
    this->progressDialog->setWindowTitle(tr("Downloading Progress"));
    this->progressDialog->setLabelText(tr("Downloading %1, please wait...").arg(file.fileName()));
    this->progressDialog->setCancelButton(nullptr); // 禁用取消按钮
    this->progressDialog->setRange(0, 100);
    this->progressDialog->setMinimumDuration(0);
    this->progressDialog->setWindowModality(Qt::WindowModal);
}

void FileDownloader::updateProgressDialog(qint64 recv, qint64 total) {
    if (this->progressDialog == nullptr) return;
    if(total > 0) {
        int percent = static_cast<int>((recv * 100) / total);
        this->progressDialog->setValue(percent);
        this->progressDialog->setLabelText(tr("Progress: %1 / %2 (%3%%)")
            .arg(FileDownloader::formatSize(recv))
            .arg(FileDownloader::formatSize(total))
            .arg(percent));
    } else {
        this->progressDialog->setMaximum(0); // 未知文件大小时显示忙碌指示
    }
    QCoreApplication::processEvents(); // 强制刷新GUI
}

void FileDownloader::cleanProgressDialog() {
    if (this->progressDialog == nullptr) return;
    this->progressDialog->close();
    delete this->progressDialog;
}
