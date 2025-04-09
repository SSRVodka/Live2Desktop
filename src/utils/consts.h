/**
 * @file consts.h
 * @brief Constants & platform headers for the application.
 * 
 * @author SSRVodka
 * @date   Mar 27, 2025
 */

#pragma once

#include <cstdint>
#include <ctime>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

// Helper function to check if a file exists
inline bool fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

#define appName "Live2Desktop"

#define RESOURCE_ROOT_DIR "Resources/"
#define CONFIG_FILE_PATH  RESOURCE_ROOT_DIR"config.json"
#define TRAY_ICON_PATH CONFIG_FILE_PATH

#define DATETIME_FORMAT "%04d-%02d-%02d %02d:%02d:%02d"

#define DEFAULT_FADE_IN_TIME 0.5
#define DEFAULT_FADE_OUT_TIME 0.5

#define IDLE_SUFFIX " (idle)"

/* --- Toast Bar Parameters --- */

/* Unit: Millisecond */
const int defaultDuration = 2000;
const int scrollInDuration = 300;
const int scrollOutDuration = 300;
const int popupHeight = 60;
const double maxOpacity = 1.0;

/* --- Model Audio Parameters --- */

const float        LIP_SYNC_RMS_WEIGHT = 6.4;

const uint32_t     MODEL_CAP_SAMPLE_RATE = 16000;
const uint32_t     MODEL_CAP_CHANNEL = 1;
const uint32_t     MODEL_CAP_BITRATE = 16;
#define            MODEL_CAP_CODEC "pcm_s16le"
#define            MODEL_CAP_CODEC_QT "audio/pcm"
#define            MODEL_CAP_CONTAINER_FORMAT "wav"
#define            MODEL_CAP_SUFFIX ".wav"

#define AUDIO_FILE_DIR "user_audio/"
#define AUDIO_GEN_DIR "gen_audio/"
// without suffix
#define AUDIO_FILENAME_LEN 16

namespace TTS {
struct tts_params_t {
    std::string server_url;
    std::string api_key;
    std::string model;
    std::string voice;
};
};

#define MODULE_STT_MODEL_LOCAL_PROTOCOL "approot://"
#define MODULE_CONFIG_FILE_PATH "config/module_config.json"

/* --- Model Chat Configurations --- */
#define MCP_CONFIG_FILE_PATH "config/mcp_config.json"