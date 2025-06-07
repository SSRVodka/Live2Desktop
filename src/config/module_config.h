#pragma once

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <sv.cpp/sense-voice/include/asr_handler.hpp>

#include <mcp.cpp/include/mcp_tool.h>

#include "utils/consts.h"
#include "utils/logger.h"

using json = nlohmann::json;

namespace mcp {
class stdio_client;
class server;
};

class LLMConfig {
public:
    std::string model = "gpt-4o";
    std::string provider = "openai";
    std::string system_prompt = "";
    std::string api_key = "";
    float temperature = 0.5;
    std::string base_url = LLM_BASE_URL_DEFAULT;
    bool stream = false;
    bool enable_thinking = false;

    LLMConfig() {}

    // Create LLMConfig from JSON object
    static LLMConfig fromJson(const json& config);
    json toJson() const;
};

class ServerConfig {
public:
    std::string command;
    std::vector<std::string> args;
    std::unordered_map<std::string, std::string> env;
    bool enabled = true;
    std::vector<std::string> exclude_tools;

    ServerConfig() {}

    // Create ServerConfig from JSON object
    static ServerConfig fromJson(const json& config);
    json toJson() const;
};

class ModuleConfigManager {
public:
    
    static ModuleConfigManager *get_instance(const std::string& config_path);
    // 调用 start_enabled_mcp_servers 后，一定要在程序结束前执行回收指令
    // 否则当前主进程结束后，创建出的 MCP 服务进程不会结束，造成系统资源泄漏
    // 注：耗时操作
    static void deinit();

    bool load();
    bool save() const;

    const ASRHandler::asr_params& get_asr_params() const { return asr_; }
    const TTS::tts_params_t& get_tts_params() const { return tts_; }

    void set_asr_params(const ASRHandler::asr_params& params) { asr_ = params; }
    void set_tts_params(const TTS::tts_params_t& params) { tts_ = params; }

    LLMConfig get_llm_config() const;


    /* --------- MCP related --------- */

    bool is_mcp_enabled() const;

    // @return {mcp_host, mcp_port}
    std::pair<std::string, int> get_mcp_server_info() const;

    // 获取所有前端 MCP server 中注册的 MCP tools。包括用户给定的所有 backend MCP server 提供的服务
    std::vector<mcp::tool> get_mcp_tools() const;
    // Get only enabled server configurations
    std::unordered_map<std::string, ServerConfig> get_enabled_mcp_backend_servers() const;
    // 同时启动后端 MCP servers 以及注册好的前端 MCP server
    bool start_enabled_mcp_servers();

private:

    explicit ModuleConfigManager(const std::string& config_path);

    // 这同时会清空 mcp_backend_servers、mcp_server，然后 disable mcp。如果需要使用则需要重新 load()
    void stop_and_cleanup_mcp_servers(bool async = false);
    void __sync_stop_and_cleanup_mcp_servers();
    // 异步删除需要注意不能在程序快结束时调用。因为异步线程是 detached 的，程序退出会导致资源回收不充分
    void _async_stop_and_cleanup_mcp_servers();

    mcp::stdio_client *build_mcp_ioclient_from_server_config(const ServerConfig &config);

    std::string config_path_;
    ASRHandler::asr_params asr_;
    TTS::tts_params_t tts_;
    LLMConfig llm_config;

    std::string mcp_addr;
    int mcp_port;
    bool use_mcp;

    // 注意：这里 stdio_client 的作用是启动 MCP 后端服务进程，与用户指定的 MCP 进程一一对应
    typedef std::pair<ServerConfig, mcp::stdio_client*> MCPServerInstance;
    // 后端用户指定的 MCP servers
    std::unordered_map<std::string, MCPServerInstance> mcp_backend_servers;

    // 注：这里是前端 MCP server，作用是汇总所有用户指定的、通过 stdio 启动的 MCP 进程提供的服务的接口
    mcp::server *mcp_server;
    
    static ModuleConfigManager *instance;
    // 是否已加载配置
    static bool isLoaded;
};
