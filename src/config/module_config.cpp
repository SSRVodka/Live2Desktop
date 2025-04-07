
#include "config/module_config.h"
#include "utils/logger.h"

ModuleConfigManager::ModuleConfigManager(const std::string &config_path)
: config_path_(config_path) {}

ModuleConfigManager *ModuleConfigManager::instance = nullptr;

ModuleConfigManager *ModuleConfigManager::get_instance(const std::string& config_path) {
    if (ModuleConfigManager::instance == nullptr) {
        ModuleConfigManager::instance = new ModuleConfigManager(config_path);
    }
    return ModuleConfigManager::instance;
}

bool ModuleConfigManager::load() {
    try {
        std::ifstream f(config_path_);
        json data = json::parse(f);

        if (data.contains("stt")) {
            json stt = data["stt"];
            if (stt.contains("model")) {
                std::string model_res_str = stt["model"];
                std::size_t start;
                if ((start = model_res_str.find_first_of(MODULE_STT_MODEL_LOCAL_PROTOCOL)) == std::string::npos) {
                    stdLogger.Exception("sorry, currently not support remote STT server");
                } else {
                    asr_.model = model_res_str.substr(start + strlen(MODULE_STT_MODEL_LOCAL_PROTOCOL));
                    if (!fileExists(asr_.model)) {
                        stdLogger.Exception("local STT model not found. please get it from https://huggingface.co/lovemefan/sense-voice-gguf and put one of the model under bin/models/");
                    }
                }
            }
            if (stt.contains("language")) asr_.language = stt["language"];
        }

        if (data.contains("tts")) {
            json tts = data["tts"];
            if (tts.contains("server_url")) tts_.server_url = tts["server_url"];
            if (tts.contains("api_key")) tts_.api_key = tts["api_key"];
            if (tts.contains("model")) tts_.model = tts["model"];
            if (tts.contains("voice")) tts_.voice = tts["voice"];
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool ModuleConfigManager::save() const {
    try {
        json data;

        data["stt"]["model"] = MODULE_STT_MODEL_LOCAL_PROTOCOL + asr_.model;
        data["stt"]["language"] = asr_.language;

        data["tts"]["server_url"] = tts_.server_url;
        data["tts"]["api_key"] = tts_.api_key;
        data["tts"]["model"] = tts_.model;
        data["tts"]["voice"] = tts_.voice;

        std::ofstream f(config_path_);
        f << data.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}
