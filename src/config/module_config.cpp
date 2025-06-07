
#include <mcp.cpp/include/mcp_stdio_client.h>
#include <mcp.cpp/include/mcp_server.h>

#include "config/module_config.h"
#include "utils/logger.h"

ModuleConfigManager::ModuleConfigManager(const std::string &config_path)
: config_path_(config_path), mcp_server(nullptr) {}

ModuleConfigManager *ModuleConfigManager::instance = nullptr;
bool ModuleConfigManager::isLoaded = false;

ModuleConfigManager *ModuleConfigManager::get_instance(const std::string& config_path) {
    if (ModuleConfigManager::instance == nullptr) {
        ModuleConfigManager::instance = new ModuleConfigManager(config_path);
    }
    return ModuleConfigManager::instance;
}

void ModuleConfigManager::deinit() {
    if (ModuleConfigManager::instance == nullptr) {
        return;
    }
    // 同步清理，耗时操作
    ModuleConfigManager::instance->stop_and_cleanup_mcp_servers();
    delete ModuleConfigManager::instance;
    ModuleConfigManager::instance = nullptr;
}

bool ModuleConfigManager::load() {
    // 防止 reload 时内存泄漏（异步清理）
    this->stop_and_cleanup_mcp_servers(true);
    try {
        std::ifstream f(config_path_);
        json data = json::parse(f);

        if (!data.contains("stt")) {
            stdLogger.Exception("stt not configured");
            return false;
        }
        if (!data.contains("tts")) {
            stdLogger.Exception("tts not configured");
            return false;
        }
        if (!data.contains("llm")) {
            stdLogger.Exception("llm not configured");
            return false;
        }
        if (!data.contains("mcp")) {
            stdLogger.Warning("mcp not configured. MCP will not enabled");
            this->use_mcp = false;
            mcp_addr = "";
            mcp_port = 0;
        }

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

        json tts = data["tts"];
        if (tts.contains("server_url")) tts_.server_url = tts["server_url"];
        if (tts.contains("api_key")) tts_.api_key = tts["api_key"];
        if (tts.contains("model")) tts_.model = tts["model"];
        if (tts.contains("voice")) tts_.voice = tts["voice"];

        json llm = data["llm"];
        stdLogger.Debug("loading llm config");
        llm_config = LLMConfig::fromJson(llm);

        if (data.contains("mcp")) {

            json mcp = data["mcp"];

            if (mcp.contains("enable") && !bool(mcp["enable"])) {
                this->use_mcp = false;
            } else {
                stdLogger.Debug("loading mcp config");
                this->use_mcp = true;
                if (mcp.contains("listen_addr")) mcp_addr = mcp["listen_addr"];
                else mcp_addr = MCP_SERVER_LISTEN_DEFAULT;
                if (mcp.contains("server_port")) mcp_port = mcp["server_port"];
                else mcp_port = MCP_SERVER_PORT_DEFAULT;

                
                // 先构造前端 MCP server
                this->mcp_server = new mcp::server(this->mcp_addr, this->mcp_port,
                    appName " Frontend MCP Server", mcp::MCP_VERSION);
                // enable tools capability
                this->mcp_server->set_capabilities({
                    {"tools", mcp::json::object()}
                });

                if (data.contains("mcpServers")) {
                    json mcp_svrs = data["mcpServers"];
                    ServerConfig current_config;
                    mcp::stdio_client *current_client;
                    for (const auto& [name, serverConfigJson] : mcp_svrs.items()) {
                        current_config = ServerConfig::fromJson(serverConfigJson);
                        if (current_config.enabled) {
                            current_client = this->build_mcp_ioclient_from_server_config(current_config);
                        } else {
                            current_client = nullptr;
                            stdLogger.Debug("mcp backend server '" + name + "' is not enabled");
                        }
                        mcp_backend_servers[name] = std::make_pair(current_config, current_client);
                    }
                }
            }
        }

        ModuleConfigManager::isLoaded = true;
        return true;
    } catch (std::exception &ex) {
        stdLogger.Exception(std::string("error loading module configurations: ") + ex.what());
        return false;
    }
}

bool ModuleConfigManager::save() const {
    if (!ModuleConfigManager::isLoaded) {
        stdLogger.Exception("configuration not loaded. Skipped saving");
        return false;
    }
    try {
        json data;

        data["stt"]["model"] = MODULE_STT_MODEL_LOCAL_PROTOCOL + asr_.model;
        data["stt"]["language"] = asr_.language;

        data["tts"]["server_url"] = tts_.server_url;
        data["tts"]["api_key"] = tts_.api_key;
        data["tts"]["model"] = tts_.model;
        data["tts"]["voice"] = tts_.voice;

        data["llm"] = llm_config.toJson();

        if (this->is_mcp_enabled()) {
            data["mcp"]["listen_addr"] = mcp_addr;
            data["mcp"]["server_port"] = mcp_port;

            for (const auto &kv: mcp_backend_servers) {
                const MCPServerInstance &mcp_srv_instance = kv.second;
                data["mcpServers"][kv.first] = mcp_srv_instance.first.toJson();
            }
        }

        std::ofstream f(config_path_);
        f << data.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

LLMConfig ModuleConfigManager::get_llm_config() const {
    if (!ModuleConfigManager::isLoaded) {
        stdLogger.Exception("configurations not loaded when get llm config");
        return LLMConfig{};
    }
    return this->llm_config;
}

bool ModuleConfigManager::is_mcp_enabled() const {
    return ModuleConfigManager::isLoaded && this->use_mcp;
}

std::vector<mcp::tool> ModuleConfigManager::get_mcp_tools() const {
    if (!ModuleConfigManager::isLoaded) {
        stdLogger.Exception("configurations not loaded when get mcp tools");
        return {};
    }
    return this->mcp_server->get_tools();
}

std::unordered_map<std::string, ServerConfig> ModuleConfigManager::get_enabled_mcp_backend_servers() const {
    if (!ModuleConfigManager::isLoaded) {
        stdLogger.Exception("configurations not loaded when get enabled mcp backend server");
        return {};
    }
    std::unordered_map<std::string, ServerConfig> enabledServers;
    for (const auto& [name, mcp_svr_instance] : mcp_backend_servers) {
        if (mcp_svr_instance.first.enabled) {
            enabledServers[name] = mcp_svr_instance.first;
        }
    }
    return enabledServers;
}

void ModuleConfigManager::start_enabled_mcp_servers() {
    if (!this->is_mcp_enabled()) {
        stdLogger.Warning("mcp not enabled. Skip starting mcp servers");
        return;
    }
    // 启动所有 stdio client 以及对应的 MCP 服务进程
    for (const auto& [name, mcp_svr_instance] : ModuleConfigManager::instance->mcp_backend_servers) {
        if (!mcp_svr_instance.first.enabled) continue;
        mcp::stdio_client *current_client = mcp_svr_instance.second;
        if (!current_client->initialize(name, mcp::MCP_VERSION)) {
            stdLogger.Exception("failed to start mcp backend server: " + name);
            continue;
        }
        stdLogger.Info("mcp server '" + name + "' started");

        // 向前端 MCP server 注册后端用户指定的 MCP server 的服务，完成服务汇总
        auto stdio_client_tools = current_client->get_tools();
        for (const auto &tool: stdio_client_tools) {
            std::string current_tool_name = tool.name;
            this->mcp_server->register_tool(
                tool,
                // current_client 是堆上指针，在 deinit 前会一直存活，因此可以值传递
                [current_client, current_tool_name, name]
                (const json &params, const std::string&/* session id */)->json{
                    stdLogger.Debug(
                        QString::asprintf("tool '%s' in backend server '%s' is called",
                            current_tool_name.data(), name.data())
                        .toStdString());
                    return current_client->call_tool(current_tool_name, params);
                }
            );
        }
    }

    // 启动前端 MCP server
    this->mcp_server->start(false);
}

void ModuleConfigManager::stop_and_cleanup_mcp_servers(bool async) {
    if (async) {
        this->_async_stop_and_cleanup_mcp_servers();
    } else {
        this->__sync_stop_and_cleanup_mcp_servers();
    }
}

void ModuleConfigManager::__sync_stop_and_cleanup_mcp_servers() {
    if (!this->is_mcp_enabled()) {
        stdLogger.Debug("mcp not enabled. Skipped cleanning mcp servers");
        return;
    }
    // 停止前端 MCP server (析构即触发)
    delete this->mcp_server;
    this->mcp_server = nullptr;
    // 需要回收管理的 stdio client 以及对应的 MCP 服务进程
    for (const auto& [name, mcp_svr_instance] : this->mcp_backend_servers) {
        if (mcp_svr_instance.first.enabled) {
            delete mcp_svr_instance.second;
        }
    }
    this->mcp_backend_servers.clear();
    this->use_mcp = false;
}

void ModuleConfigManager::_async_stop_and_cleanup_mcp_servers() {
    if (!this->is_mcp_enabled()) {
        // stdLogger.Debug("mcp not enabled. Skipped cleanning mcp servers");
        return;
    }
    // 异步停止前端 mcp server
    if (this->mcp_server) {
        auto* server_to_delete = this->mcp_server;
        this->mcp_server = nullptr;  // 同步操作，确保后续访问安全

        std::thread([server_to_delete]() {
            delete server_to_delete;  // 后台线程中阻塞删除
        }).detach();
    }

    // 异步删除后端 MCP 服务
    std::vector<mcp::stdio_client*> servers_to_delete;
    for (const auto& [name, mcp_svr_instance] : this->mcp_backend_servers) {
        if (mcp_svr_instance.first.enabled) {
            servers_to_delete.push_back(mcp_svr_instance.second);
        }
    }
    this->mcp_backend_servers.clear();  // 同步清空容器，确保后续访问安全

    for (auto* server_ptr : servers_to_delete) {
        std::thread([server_ptr]() {
            delete server_ptr;
        }).detach();
    }

    this->use_mcp = false;  // 同步更新状态
}

std::pair<std::string, int> ModuleConfigManager::get_mcp_server_info() const {
    if (!ModuleConfigManager::isLoaded) {
        stdLogger.Exception("configurations not loaded when get mcp server info");
        return {"", 0};
    }
    return std::make_pair(mcp_addr, mcp_port);
}

// caller should check command not empty
mcp::stdio_client *ModuleConfigManager::build_mcp_ioclient_from_server_config(const ServerConfig &config) {
    std::string fullCmd = config.command;
    for (const auto &arg: config.args) {
        fullCmd.append(" ");
        fullCmd.append(arg);
    }
    return new mcp::stdio_client(fullCmd, config.env);
}


LLMConfig LLMConfig::fromJson(const json &config) {
    LLMConfig llmConfig;
    llmConfig.model = config.value("model", llmConfig.model);
    llmConfig.provider = config.value("provider", llmConfig.provider);
    llmConfig.system_prompt = config.value("system_prompt", "you are a helpful assistant");
    llmConfig.api_key = config.contains("api_key") ? config.value("api_key", llmConfig.api_key) : (
                            std::getenv("LLM_API_KEY") ? std::string(std::getenv("LLM_API_KEY")) : (
                                std::getenv("OPENAI_API_KEY") ? std::getenv("OPENAI_API_KEY") : ""
                            )
                        );
    llmConfig.temperature = config.value("temperature", llmConfig.temperature);
    llmConfig.base_url = config.value("base_url", llmConfig.base_url);
    llmConfig.stream = config.contains("stream") ? std::optional<bool>(config["stream"]).value_or(false) : false;
    llmConfig.enable_thinking = config.contains("enable_thinking") ? std::optional<bool>(config["enable_thinking"]).value_or(false) : false;
    return llmConfig;
}

json LLMConfig::toJson() const {
    json j;
    j["model"] = model;
    j["provider"] = provider;
    j["system_prompt"] = system_prompt;
    j["api_key"] = api_key;
    j["temperature"] = temperature;
    j["base_url"] = base_url;
    j["stream"] = stream;
    j["enable_thinking"] = enable_thinking;
    return j;
}

ServerConfig ServerConfig::fromJson(const json& config) {
    ServerConfig serverConfig;
    serverConfig.command = config.at("command");
    serverConfig.args = config.value("args", std::vector<std::string>());
    serverConfig.env = config.value("env", std::unordered_map<std::string, std::string>());
    serverConfig.enabled = config.value("enabled", true);
    serverConfig.exclude_tools = config.value("exclude_tools", std::vector<std::string>());
    return serverConfig;
}

json ServerConfig::toJson() const {
    json j;
    j["command"] = command;
    j["args"] = args;
    j["env"] = env;
    j["enabled"] = enabled;
    j["exclude_tools"] = exclude_tools;
    return j;
}
