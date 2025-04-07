#include <QtWidgets/QApplication>

#include "config/mcp_config.h"
#include "utils/consts.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    chdir(QCoreApplication::applicationDirPath().toStdString().c_str());
    try {
        MCPConfig *config = MCPConfig::getInstance();
        if (!config->load({MCP_CONFIG_FILE_PATH})) {
            throw new std::runtime_error("failed to load mcp configurations");
        }
        std::cout << "Loaded configuration successfully.\n";

        auto enabledServers = config->getEnabledServers();
        std::cout << "Enabled servers:\n";
        for (const auto& [name, serverConfig] : enabledServers) {
            std::cout << " - " << name << ": " << serverConfig.command << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    }

    return 0;
}
