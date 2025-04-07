#include <QtCore/QFileInfo>
#include <QtCore/QUrl>

#include "modules/audio/audio_recorder.h"
#include "modules/audio/audio_handler.h"

#include "utils/consts.h"
#include "utils/logger.h"


AudioRecorder::AudioRecorder(AudioHandler *p_handler): QObject(nullptr), handler(p_handler), recording(false) {
    // default: little-endian
    // recorder.setContainerFormat(QString::fromStdString(MODEL_CAP_CONTAINER_FORMAT));
    // settings.setCodec(QString::fromStdString(MODEL_CAP_CODEC_QT));
    settings.setSampleRate(MODEL_CAP_SAMPLE_RATE);
    settings.setChannelCount(MODEL_CAP_CHANNEL);
    settings.setBitRate(MODEL_CAP_BITRATE);
    settings.setQuality(QMultimedia::NormalQuality);
    recorder.setAudioSettings(settings);

    this->setRecordThreshold(); // use default value

    connect(&player, 
        QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
        this,
        &AudioRecorder::handleMediaError);
}

AudioRecorder::~AudioRecorder() {
    if (this->recording) this->record_stop();
}

void AudioRecorder::record() {
    if (this->recording) {
        stdLogger.Warning("recorder is already recording. Operation ignored");
        return;
    }
    this->current_file = AudioHandler::get_new_audio_filename(true);
    recorder.setOutputLocation(QUrl::fromLocalFile(this->current_file));
    stdLogger.Info("Start recording audio...");
    this->recording = true;
    this->last_record_timestamp = time(NULL);
    recorder.record();
}

QString AudioRecorder::record_stop() {
    if (!this->recording) {
        stdLogger.Warning("stop an inactive recorder. Operation ignored");
        return QString();
    }
    this->recording = false;
    recorder.stop();
    if (time(NULL) - this->last_record_timestamp < this->record_threshold) {
        // record time is less than threshold: ignored
        stdLogger.Warning("record duration is shorter than threshold: ignored");
        return QString();
    }
    QString msg = QString("Stop recording audio. Data written to %1")
        .arg(recorder.outputLocation().fileName());
    std::string conv = msg.toStdString();
    stdLogger.Info(conv.c_str());

    // convert it to model compatible format
    std::string cur_fstr = this->current_file.toStdString();
    std::string converted_file = cur_fstr +
        MODEL_CAP_CODEC + MODEL_CAP_SUFFIX;
    if (this->handler->audio2modelwav(cur_fstr, converted_file)) {
        return QString::fromStdString(converted_file);
    }
    stdLogger.Exception("failed to convert audio file to model compatible format");
    return QString();
}

void AudioRecorder::setRecordThreshold(unsigned int threshold) {
    this->record_threshold = threshold;
}

void AudioRecorder::play(const QString &audio_file) {
    player.setMedia(QUrl::fromLocalFile(QFileInfo(audio_file).absoluteFilePath()));
    player.play();
}

void AudioRecorder::handleMediaError(QMediaPlayer::Error error) {
    std::string msg = "media player error: ";
    switch (error) {
    case QMediaPlayer::ResourceError:
        msg += "resource error (file not exists or format not support)";
        break;
    case QMediaPlayer::FormatError:
        msg += "audio format not supported";
        break;
    case QMediaPlayer::NetworkError:
        msg += "network stream error";
        break;
    default:
        msg += "unknown error";
    }
    stdLogger.Exception(msg.c_str());
}
