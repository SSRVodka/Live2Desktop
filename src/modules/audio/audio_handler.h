/**
 * @file audio_handler.h
 * @brief Helper class for audio resources
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */

#pragma once

#include <cstdlib>
#include <QtCore/QFile>

#include <sv.cpp/sense-voice/include/asr_handler.hpp>
#include "utils/consts.h"

// forward declarations
class AudioRecorder;

namespace STT {
class Client;
};
namespace TTS {
class Client;
};

/**
 * @brief handle audio stuff like:
 *  - audio file storage
 *  - audio format convertion
 *  - audio recorder (recording & playing)
 *  - (optional) Speech to text (STT)
 *  - (optional) Text to speech (TTS)
 */
class AudioHandler: public QObject {
    Q_OBJECT
public:

    AudioHandler(QObject *parent = nullptr);
    ~AudioHandler();

    typedef ASRHandler::asr_params stt_params_t;
    typedef TTS::tts_params_t tts_params_t;

    void set_stt_params(const stt_params_t &params) { this->stt_params = params; }
    void set_tts_params(const tts_params_t &params) { this->tts_params = params; }

    /**
     * @brief get the inner recorder object pointer. The owner is AudioHandler (`this`)
     */
    AudioRecorder *get_recorder_unsafe_ptr() const { return this->recorder; }

    /**
     * @brief convert audio source to text (STT) asynchronously.
     *  Listen to signal `stt_reply` to get result.
     * 
     * @see AudioHandler::stt_reply
     */
    void stt_request(const QString &audio_file);
    /**
     * @brief convert text to audio source (TTS) asynchronously.
     *  Listen to signal `tts_reply` to get result.
     * @return generated audio filename.
     * @note you should NOT access the returned file util `tts_reply` is sent.
     * 
     * @see AudioHandler::tts_reply
     */
    QString tts_request(const QString &text);

    /**
     * @brief generate a random filename for audio source.
     * @param isUser is this audio source generated from user
     * @return the random audio filename
     */
    static QString get_new_audio_filename(bool isUser);

    /**
     * Convert any valid audio format to model compatible format (*.wav). Default:
     * - audio sample rate: 16000
     * - audio channel: 1
     * - codec: -c:a pcm_s16le (PCM 16-bit little endian)
     * 
     * @param inputfn Input filename
     * @param outputfn Output filename
     * @return Whether the convertion is successful or not
     */
    static bool audio2modelwav(std::string inputfn, std::string outputfn);

    // TODO: replace expedient with media process logic
    static bool check_ffmpeg_exists() {
        #ifdef _WIN32
            return system("where ffmpeg >nul 2>nul") == 0;
        #else
            return system("which ffmpeg >/dev/null 2>&1") == 0;
        #endif
    }
signals:
    void stt_reply(bool valid, QString transcribed_text);
    void tts_reply(bool success, QString msg);

private:
    stt_params_t stt_params;
    tts_params_t tts_params;

    AudioRecorder *recorder;
    STT::Client *stt_client;
    TTS::Client *tts_client;
};
