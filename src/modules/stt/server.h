#pragma once

#include <httplib.cpp/httplib.h>

namespace STT {

// string enum
namespace OutputType {
    // output formats
    const std::string json_format   = "json";
    const std::string text_format   = "text";
    const std::string srt_format    = "srt";
    const std::string vjson_format  = "verbose_json";
    const std::string vtt_format    = "vtt";
};
struct server_params {
    std::string hostname = "127.0.0.1";
    std::string public_path = "examples/server/public";
    std::string request_path = "";
    std::string inference_path = "/inference";

    int32_t port          = 8080;
    int32_t read_timeout  = 600;
    int32_t write_timeout = 600;

    bool ffmpeg_converter = false;
};
struct whisper_params {
    int32_t n_threads     = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_processors  = 1;
    int32_t offset_t_ms   = 0;
    int32_t offset_n      = 0;
    int32_t duration_ms   = 0;
    int32_t progress_step = 5;
    int32_t max_context   = -1;
    int32_t max_len       = 0;
    int32_t best_of       = 2;
    int32_t beam_size     = -1;
    int32_t audio_ctx     = 0;

    float word_thold      =  0.01f;
    float entropy_thold   =  2.40f;
    float logprob_thold   = -1.00f;
    float temperature     =  0.00f;
    float temperature_inc =  0.20f;
    float no_speech_thold = 0.6f;

    bool debug_mode      = false;
    bool translate       = false;
    bool detect_language = false;
    bool diarize         = false;
    bool tinydiarize     = false;
    bool split_on_word   = false;
    bool no_fallback     = false;
    bool print_special   = false;
    bool print_colors    = false;
    bool print_realtime  = false;
    bool print_progress  = false;
    bool no_timestamps   = false;
    bool use_gpu         = true;
    bool flash_attn      = false;
    bool suppress_nst    = false;

    std::string language        = "en";
    std::string prompt          = "";
    std::string font_path       = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf";
    std::string model           = "models/ggml-base.en.bin";

    std::string response_format     = OutputType::json_format;

    // [TDRZ] speaker turn string
    std::string tdrz_speaker_turn = " [SPEAKER_TURN]"; // TODO: set from command line

    std::string openvino_encode_device = "CPU";

    std::string dtw = "";
};

class Server {
public:

    struct whisper_print_user_data {
        const whisper_params * params;
    
        const std::vector<std::vector<float>> * pcmf32s;
        int progress_prev;
    };

    int cli_main(int argc, char *argv[]);


protected:
    static void whisper_print_usage(int /*argc*/, char ** argv, const whisper_params & params, const server_params& sparams);
    static bool whisper_params_parse(int argc, char ** argv, whisper_params & params, server_params & sparams);

    static bool parse_str_to_bool(const std::string & s) {
        if (s == "true" || s == "1" || s == "yes" || s == "y") {
            return true;
        }
        return false;
    }

    static void get_req_parameters(const httplib::Request & req, whisper_params & params);

    static void check_ffmpeg_availibility();
    static std::string generate_temp_filename(const std::string &prefix, const std::string &extension);
    static bool convert_to_wav(const std::string & temp_filename, std::string & error_resp);
    static std::string estimate_diarization_speaker(std::vector<std::vector<float>> pcmf32s, int64_t t0, int64_t t1, bool id_only = false);

    static void whisper_print_progress_callback(struct whisper_context * /*ctx*/, struct whisper_state * /*state*/, int progress, void * user_data);
    static void whisper_print_segment_callback(struct whisper_context * ctx, struct whisper_state * /*state*/, int n_new, void * user_data);

    static std::string output_str(struct whisper_context * ctx, const whisper_params & params, std::vector<std::vector<float>> pcmf32s);
};


};
