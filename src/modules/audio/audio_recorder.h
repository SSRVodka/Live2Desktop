/**
 * @file audio_recorder.h
 * @brief Audio recorder class for recording & playing audio sources
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */


#pragma once

#include <cstring>
#include <string>

#include <QtMultimedia/QAudioRecorder>
#include <QtMultimedia/QMediaPlayer>

class AudioHandler;

class AudioRecorder: public QObject {
    Q_OBJECT
public:

    AudioRecorder(AudioHandler *p_handler);
    ~AudioRecorder();

    void setRecordThreshold(unsigned int threshold = 3 /* second(s) */);

    void record();
    /**
     * @return empty string if error occurs
     */
    QString record_stop();
    void play(const QString &audio_file);
protected slots:
    void handleMediaError(QMediaPlayer::Error error);
private:
    AudioHandler *handler;

    QMediaPlayer player;
    QAudioRecorder recorder;
    QAudioEncoderSettings settings;

    long record_threshold;
    long last_record_timestamp;
    QString current_file;
    bool recording;
};
