
#include <random>
#include <sstream>

#include <QtCore/QDir>

#include "modules/stt/client.h"
#include "modules/tts/client.h"

#include "modules/audio/audio_handler.h"
#include "modules/audio/audio_recorder.h"

#include "utils/logger.h"


AudioHandler::AudioHandler(QObject *parent)
    : QObject(parent),
    recorder(new AudioRecorder(this)),
    stt_client(new STT::Client(this)),
    tts_client(new TTS::Client(this)) {
    
    connect(this->stt_client, SIGNAL(replyArrived(bool,const QString&)),
            this, SIGNAL(stt_reply(bool,QString)));
    connect(this->tts_client, SIGNAL(finished(bool,const QString&)),
            this, SIGNAL(tts_reply(bool,QString)));
}

AudioHandler::~AudioHandler() {
    // parent never ref them
    delete this->recorder;
    delete this->tts_client;
    delete this->stt_client;
}

void AudioHandler::stt_request(const QString &audio_file) {
    // prepare stt parameters
    this->stt_params.fname_inp.clear();
    this->stt_params.fname_inp.emplace_back(audio_file.toStdString());

    this->stt_client->sendWav(this->stt_params);
}

QString AudioHandler::tts_request(const QString &text) {
    // generate audio filename
    QString gen_audio_fn = AudioHandler::get_new_audio_filename(false);

    this->tts_client->generateSpeech(this->tts_params, text, gen_audio_fn);
    return gen_audio_fn;
}

QString AudioHandler::get_new_audio_filename(bool isUser) {
    const std::string charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789";
    
    static std::random_device rd;
    static unsigned seed = rd() ^ std::chrono::system_clock::now().time_since_epoch().count();
    static thread_local std::mt19937 generator(seed);
    static std::uniform_int_distribution<int> distribution(0, charset.size() - 1);

    // check if directory exists (if not then create it)
    QString audio_dir_str(AUDIO_FILE_DIR);
    QString audio_gen_dir_str(AUDIO_GEN_DIR);
    QDir audio_dir(audio_dir_str);
    QDir audio_gen_dir(audio_gen_dir_str);
    if (!audio_dir.exists()) {
        if (!audio_dir.mkpath(AUDIO_FILE_DIR)) {
            stdLogger.Exception("failed to create directory (" AUDIO_FILE_DIR ") for audio files");
        }
    }
    if (!audio_gen_dir.exists()) {
        if (!audio_gen_dir.mkpath(AUDIO_GEN_DIR)) {
            stdLogger.Exception("failed to create directory (" AUDIO_GEN_DIR ") for audio files");
        }
    }
    
    std::string filename;
    filename.clear();
    filename.reserve(AUDIO_FILENAME_LEN + 4 /* SUFFIX LEN */);
    for (int i = 0; i < AUDIO_FILENAME_LEN; ++i) {
        filename += charset[distribution(generator)];
    }
    filename += MODEL_CAP_SUFFIX;
    
    return (isUser ? audio_dir_str : audio_gen_dir_str) + QString::fromStdString(filename);
}

bool AudioHandler::audio2modelwav(std::string inputfn, std::string outputfn) {
    if (!check_ffmpeg_exists()) {
        stdLogger.Exception("ffmpeg not found on host");
        stdLogger.Warning("please install ffmepg and make sure it is under $PATH. Operation aborted.");
        return false;
    }
    
    std::ostringstream oss;
    oss << "ffmpeg -i " << inputfn
        << " -ar " << MODEL_CAP_SAMPLE_RATE
        << " -ac " << MODEL_CAP_CHANNEL
        << " -c:a " << std::string(MODEL_CAP_CODEC) << " " << outputfn
        << " -y -hide_banner -loglevel error";
    std::string command = oss.str();
    int ret = system(command.c_str());
    if (ret) {
        std::string msg = "ffmpeg exited with code: " + std::to_string(ret);
        stdLogger.Exception(msg.c_str());
        return false;
    }
    else return true;
}


