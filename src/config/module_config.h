#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <sv.cpp/sense-voice/include/asr_handler.hpp>

#include "utils/consts.h"

using json = nlohmann::json;


class ModuleConfigManager {
public:
    
    static ModuleConfigManager *get_instance(const std::string& config_path);

    bool load();
    bool save() const;

    const ASRHandler::asr_params& get_asr_params() const { return asr_; }
    const TTS::tts_params_t& get_tts_params() const { return tts_; }

    void set_asr_params(const ASRHandler::asr_params& params) { asr_ = params; }
    void set_tts_params(const TTS::tts_params_t& params) { tts_ = params; }

private:

    explicit ModuleConfigManager(const std::string& config_path);

    std::string config_path_;
    ASRHandler::asr_params asr_;
    TTS::tts_params_t tts_;

    static ModuleConfigManager *instance;
};
