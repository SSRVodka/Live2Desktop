#pragma once

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "utils/consts.h"

using json = nlohmann::json;

class LLMConfig {
public:
    std::string model = "gpt-4o";
    std::string provider = "openai";
    std::optional<std::string> api_key;
    float temperature = 0.0;
    std::optional<std::string> base_url;

    // Constructor
    LLMConfig() {}

    // Create LLMConfig from JSON object
    static LLMConfig fromJson(const json& config) {
        LLMConfig llmConfig;
        llmConfig.model = config.value("model", llmConfig.model);
        llmConfig.provider = config.value("provider", llmConfig.provider);
        llmConfig.api_key = config.contains("api_key") ? std::optional<std::string>(config["api_key"]) 
                            : std::optional<std::string>(std::getenv("LLM_API_KEY") ? std::getenv("LLM_API_KEY") 
                            : std::getenv("OPENAI_API_KEY") ? std::getenv("OPENAI_API_KEY") : "");
        llmConfig.temperature = config.value("temperature", llmConfig.temperature);
        llmConfig.base_url = config.contains("base_url") ? std::optional<std::string>(config["base_url"]) : std::nullopt;
        return llmConfig;
    }
};

class ServerConfig {
public:
    std::string command;
    std::vector<std::string> args;
    std::unordered_map<std::string, std::string> env;
    bool enabled = true;
    std::vector<std::string> exclude_tools;
    std::vector<std::string> requires_confirmation;

    // Constructor
    ServerConfig() {}

    // Create ServerConfig from JSON object
    static ServerConfig fromJson(const json& config) {
        ServerConfig serverConfig;
        serverConfig.command = config.at("command");
        serverConfig.args = config.value("args", std::vector<std::string>());
        serverConfig.env = config.value("env", std::unordered_map<std::string, std::string>());
        serverConfig.enabled = config.value("enabled", true);
        serverConfig.exclude_tools = config.value("exclude_tools", std::vector<std::string>());
        serverConfig.requires_confirmation = config.value("requires_confirmation", std::vector<std::string>());
        return serverConfig;
    }
};

class MCPConfig {
public:
    LLMConfig llm;
    std::string system_prompt;
    std::unordered_map<std::string, ServerConfig> mcp_servers;
    std::vector<std::string> tools_requires_confirmation;

    static MCPConfig *getInstance() {
        if (MCPConfig::instance == nullptr) {
            MCPConfig::instance = new MCPConfig;
        }
        return MCPConfig::instance;
    }

    // Load configuration from file
    bool load(const std::vector<std::string> &configPaths) {
        std::string chosenPath;

        for (const auto& path : configPaths) {
            if (fileExists(path)) {
                chosenPath = path;
                break;
            }
        }

        if (chosenPath.empty()) {
            throw std::runtime_error("Could not find config file in any of the specified paths.");
        }

        std::ifstream file(chosenPath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + chosenPath);
        }

        json config;
        file >> config;
        file.close();

        llm = LLMConfig::fromJson(config.value("llm", json::object()));
        system_prompt = config.at("systemPrompt");

        for (const auto& [name, serverConfigJson] : config.at("mcpServers").items()) {
            mcp_servers[name] = ServerConfig::fromJson(serverConfigJson);
            const auto& requires_confirmation = serverConfigJson.value("requires_confirmation", std::vector<std::string>());
            tools_requires_confirmation.insert(tools_requires_confirmation.end(),
                                            requires_confirmation.begin(), requires_confirmation.end());
        }

        return true;
    }

    // Get only enabled server configurations
    std::unordered_map<std::string, ServerConfig> getEnabledServers() const {
        std::unordered_map<std::string, ServerConfig> enabledServers;
        for (const auto& [name, config] : mcp_servers) {
            if (config.enabled) {
                enabledServers[name] = config;
            }
        }
        return enabledServers;
    }
private:
    // private constructor
    MCPConfig();
    static MCPConfig *instance;
};
