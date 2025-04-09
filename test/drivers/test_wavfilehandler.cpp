
#include <QtWidgets/QApplication>

#include "utils/consts.h"
#include "utils/logger.h"

#include "drivers/allocator.h"
#include "drivers/wavFileHandler.h"

#define TEST_WAV "test_data/test_format.wav"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    chdir(QCoreApplication::applicationDirPath().toStdString().c_str());
    Allocator allocator;
    Csm::CubismFramework::StartUp(&allocator);
    Csm::CubismFramework::Initialize();
    
    WavFileHandler handler;
    assert(handler.Start(TEST_WAV));

    int test_time = 100;
    std::string testMsg = "Current RMS: ";

    while (test_time-- > 0) {
        handler.Update(0.08);
        stdLogger.Test(testMsg + std::to_string(handler.GetRms()));
    }

    Csm::CubismFramework::Dispose();

    return 0;
}
