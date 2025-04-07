#include <QtWidgets/QApplication>
#include <iostream>

#include "config/module_config.h"
#include "utils/consts.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    chdir(QCoreApplication::applicationDirPath().toStdString().c_str());
    ModuleConfigManager *manager = ModuleConfigManager::get_instance(MODULE_CONFIG_FILE_PATH);
    try {
        if (!manager->load()) {
            throw new std::runtime_error("failed to load module configurations");
        }
        std::cout << "Loaded module configuration successfully.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    }
    return 0;
}
